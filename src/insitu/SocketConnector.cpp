// ========================================================================== //
// Copyright (c) 2014-2020 The University of Texas at Austin.                 //
// All rights reserved.                                                       //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// A copy of the License is included with this software in the file LICENSE.  //
// If your copy does not contain the License, you may obtain a copy of the    //
// License at:                                                                //
//                                                                            //
//     https://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  //
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
//                                                                            //
// ========================================================================== //

#include <iostream>
#include <sstream>
#include <cstdio>

using namespace std;

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "KeyedObject.h"
#include "SocketConnector.hpp"
#include "Volume.h"

#include <vtkNew.h>
#include <vtkDataSetReader.h>
#include <vtkImageData.h>

namespace gxy
{

WORK_CLASS_TYPE(SocketConnector::ConnectionMsg);

void
SocketConnector::Register()
{
  RegisterClass();
  ConnectionMsg::Register();
}

KEYED_OBJECT_CLASS_TYPE(SocketConnector)

SocketConnector::~SocketConnector() {}

void
SocketConnector::initialize()
{
  super::initialize();
}

Volume*
SocketConnector::findVariable(std::string name)
{
  auto it = variables.find(name);
  if (it == variables.end())
    return nullptr;
  else
    return it->second;
}

int
SocketConnector::serialSize()
{
  return super::serialSize() + 2*sizeof(int);
}

unsigned char *
SocketConnector::serialize(unsigned char *p)
{
  p = super::serialize(p);

  *(int *)p = port;
  p += sizeof(int);

  *(int *)p = wait_time;
  p += sizeof(int);

  return p;
}

unsigned char *
SocketConnector::deserialize(unsigned char *p)
{
  p = super::deserialize(p);

  port = *(int *)p;
  p += sizeof(int);

  wait_time = *(int *)p;
  p += sizeof(int);

  return p;
}

bool
SocketConnector::local_commit(MPI_Comm c)
{
  local_port = port + GetTheApplication()->GetRank();
  return false;
}

bool 
SocketConnector::has(std::string name)
{
  return findVariable(name) != nullptr;
}

void
SocketConnector::Open()
{
  ConnectionMsg msg(this, ConnectionMsg::Open);
  msg.Broadcast(true, false);
}

void
SocketConnector::Close()
{
  ConnectionMsg msg(this, ConnectionMsg::Close);
  msg.Broadcast(true, false);
}

bool
SocketConnector::local_open(MPI_Comm c)
{
  sskt = vtkServerSocket::New();
  sskt->CreateServer(port + GetTheApplication()->GetRank()) ;

  return false;
}

bool
SocketConnector::local_close(MPI_Comm c)
{
  if (sskt)
  {
    sskt->CloseSocket();
    sskt->Delete();
    sskt = NULL;
  }

  return false;
}

void
SocketConnector::Accept(VolumeP var)
{
  waiter.busy = true;
  
  ConnectionMsg msg(this, var, ConnectionMsg::Accept);
  msg.Broadcast(true, false);
}

bool SocketConnector::local_accept(MPI_Comm c, VolumeP volume)
{
  vtkClientSocket *cskt = sskt->WaitForConnection(wait_time);

  int allflag, flag = cskt == NULL  ? 0 : 1;
  MPI_Allreduce(&flag, &allflag, 1, MPI_INT, MPI_MIN, c);

  if (allflag == 0)
    return false;

  float min, max;
  int point_count;

  int i,j,k;
  volume->get_ghosted_local_counts(i, j, k);

  int sz = i*j*k * (volume->isFloat() ? sizeof(float) : sizeof(unsigned char)) * volume->get_number_of_components();

  if (! cskt->Receive(volume->get_samples(), sz, 1))
  {
    std::cerr << "error... unable to read time step\n";
    exit(0);
  }

  int one = 1;
  if (! cskt->Send(&one, sizeof(one)))
  {
    std::cerr << "error... sending ack\n";
    exit(0);
  }

  cskt->CloseSocket();
  cskt->Delete();

  return false;
}

SocketConnector::ConnectionMsg::ConnectionMsg(SocketConnector* s, VolumeP v, todo t) : ConnectionMsg(2*sizeof(Key) + sizeof(todo))
{
  unsigned char *p = contents->get();

  *(Key *)p = s->getkey();
  p += sizeof(Key);

  *(Key *)p = v->getkey();
  p += sizeof(Key);

  *(todo *)p = t;
}

SocketConnector::ConnectionMsg::ConnectionMsg(SocketConnector* s, todo t) : ConnectionMsg(2*sizeof(Key) + sizeof(todo))
{
  unsigned char *p = contents->get();

  *(Key *)p = s->getkey();
  p += sizeof(Key);

  *(Key *)p = (Key)-1;
  p += sizeof(Key);

  *(todo *)p = t;
}

bool SocketConnector::ConnectionMsg::CollectiveAction(MPI_Comm comm, bool is_root)
{
  unsigned char *p = contents->get();

  SocketConnectorP c = SocketConnector::GetByKey(*(Key *)p);
  p +=sizeof(Key);

  Key vkey = *(Key *)p;
  p +=sizeof(Key);

  VolumeP v;
  if (vkey != -1)
    v = Volume::GetByKey(vkey);

  todo t = *(todo *)p;

  bool r;
  std::string s;
  
  switch(t)
  {
    case Open:
      r = c->local_open(comm);
      s = "open";
      break;
    case Close:
      r = c->local_close(comm);
      s = "close";
      break;
    case Accept:
      r = c->local_accept(comm, v);
      s = "accept";
      break;
  }

  /*
  This looks weird.   Here's the thing: when doing an async transfer,
  the sim main loop doesn't wait for the Accept to complete; this is so
  a single sim flow of control can tell Galaxy to accept a timestep then
  immediately send it. For example, the sim loop MPI process might look like:

  while (doing timesteps) {
    compute time step synchronize

    if (root process)
      tell Galaxy to accept a timestep

    send the time step
  }

  So consider the ownership of the connection itself.  Its initially
  owned by the master thread.   Then, while the transfer is underway,
  its also owned by various bits of the communications infrastructure,
  including a local reference in the CollectiveAction that is doing
  the transfer.   Since thats collective (so that the global
  properties of the updated object can be gathered) it occurs
  directly in the message loop.   Suppose that ownership is the
  last reference on the connection; when the CollectiveAction
  completes and removes its local reference, the object will get
  deleted.

  Problem is that the deletion itself issues a collective operation
  (causing all the nodes to drop their bit of the object).   This
  will deadlock since that will never get picked up by the message
  loop (since the message loop will be waiting for it to complete).

  We need to require the explicit wait for the completion of the
  transfer so that the master doesn't remove its reference on the
  object until the transfer completes and the CollectiveAction
  reference to it has been removed.   Thus the final reference will
  be removed outside the message loop and it won't deadlock.   This
  makes the calling code look more like:

  while (doing timesteps) {
    compute time step synchronize

    if (root process)
      tell Galaxy to accept a timestep

    send the time step

    if (root process)
      wait for Galaxy to receive the time step
  }

  This also enables the caller to wait for the transfer to complete
  so that it can single buffer.

  */

  Waiter *waiter = &c->waiter;

  c = NULL;
  
  if (is_root)
  {
    pthread_mutex_lock(&waiter->lock);
    waiter->busy = false;
    pthread_cond_signal(&waiter->cond);
    pthread_mutex_unlock(&waiter->lock);
  }

  return r;
}

}
