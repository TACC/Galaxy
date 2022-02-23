// ========================================================================== //
//                                                                            //
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

static int frame = 0;

#include "Application.h"
#include "Renderer.h"
#include "Receiver.hpp"
#include "bufhdr.h"
#include "Box.h"

using namespace gxy;

#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace gxy {

WORK_CLASS_TYPE(Receiver::RunMsg)
WORK_CLASS_TYPE(Receiver::StopMsg)
WORK_CLASS_TYPE(Receiver::ReshuffleMsg)
WORK_CLASS_TYPE(Receiver::AcceptMsg)
WORK_CLASS_TYPE(Receiver::WaitMsg)
WORK_CLASS_TYPE(Receiver::HostsMsg)

KEYED_OBJECT_CLASS_TYPE(Receiver)

void
Receiver::Register()
{
  RegisterClass();
  Receiver::AcceptMsg::Register();
  Receiver::WaitMsg::Register();
  Receiver::HostsMsg::Register();
  Receiver::RunMsg::Register();
  Receiver::StopMsg::Register();
  Receiver::ReshuffleMsg::Register();
}

Receiver::~Receiver()
{
  if (state == PAUSED)
    _Stop();

  MPI_Comm_free(&receiver_comm);
}

void
Receiver::initialize()
{
}

bool
Receiver::local_commit(MPI_Comm c)
{
  if (super::local_commit(c))
    return true;

  int grnk, gsz;
  MPI_Comm_rank(c, &grnk);
  MPI_Comm_size(c, &gsz);
  MPI_Comm_dup(c, &receiver_comm);

  local_port = base_port + grnk + 1;

  return false;
}

bool
Receiver::Commit()
{
  if (! super::Commit())
    return false;

  HostsMsg msg(this);
  msg.Broadcast(true, true);

  return true;
}

void
Receiver::SetGeometry(GeometryP g)
{
  geometry = g;
}

int 
Receiver::serialSize() { return super::serialSize() + sizeof(commit_args); }

unsigned char*
Receiver::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);

  commit_args *a = (commit_args *)ptr;

  a->base_port = base_port;
  a->geometry = geometry->getkey();
  a->nsenders = nsenders;
  a->fuzz = fuzz;

  return ptr + sizeof(commit_args);
}

unsigned char*
Receiver::deserialize(unsigned char *ptr)
{
  ptr = super::deserialize(ptr);

  commit_args *a = (commit_args *)ptr;

  base_port = a->base_port;
  geometry = Geometry::GetByKey(a->geometry);
  nsenders = a->nsenders;
  fuzz = a->fuzz;

  return ptr + sizeof(commit_args);
}

void
Receiver::Stop()
{
  masterskt->stop();

  StopMsg msg(this);
  msg.Broadcast(true, true);
}

void
Receiver::_Stop()
{
  state = EXITTING;
  serverskt->stop();
}

void
Receiver::Start(bool(*handler)(ServerSkt*, void *, char*))
{
  masterskt = new ServerSkt(base_port, handler, this);
  masterskt->start(false);

  done_count = GetTheApplication()->GetSize() - 1;
}

void
Receiver::Run()
{
  RunMsg msg(this);
  msg.Broadcast(true, true);
}

bool
Receiver::Wait()
{
  lock_state();

  while (state != PAUSED && state != ERROR)
    wait_state();

  unlock_state();
  return true;
}

void
Receiver::Accept(int nsenders)
{
  AcceptMsg msg(this, nsenders);
  msg.Broadcast(true, true);
}

bool
Receiver::Setup(MPI_Comm comm)
{
  state = PAUSED;

  serverskt = new ServerSkt(local_port, receiver, this);
  serverskt->start(true);

  pthread_create(&tid, NULL, reshuffle_thread, (void *)this);

  return false;
}

bool
Receiver::Reshuffle()
{
  std::cerr << "Reshuffle start\n";

  int r = GetTheApplication()->GetRank();
  int s = GetTheApplication()->GetSize();

  PartitioningP partitioning = GetTheRenderer()->GetPartitioning();

  // First we see how much data we need to send to each recipient

  int *recipient_vertex_count = new int[s];
  memset(recipient_vertex_count, 0, s*sizeof(int));

  int *recipient_connectivity_count = new int[s];
  memset(recipient_connectivity_count, 0, s*sizeof(int));

  // For each input buffer we got from the socket...

  bufhdr::btype btype = bufhdr::None;
  bool has_data;
  int tot_points = 0;

  for (auto buffer : buffers)
  {
    int size = *(int *)buffer;
    bufhdr *hdr = (bufhdr *)(buffer + sizeof(int));

    if (btype == bufhdr::None)
    {
      btype = hdr->type;
      has_data = hdr->has_data;
    }
    else if (btype != hdr->type || has_data != hdr->has_data)
    {
      std::cerr << "difference in data received from different senders\n";
      exit(1);
    }

    float *ptr = (float *)(buffer + sizeof(int) + sizeof(bufhdr));

    if (hdr->type == bufhdr::Particles)
    {
      // Then we do it point by point.  For each point in the buffer...

      tot_points += hdr->npoints;
      for (int i = 0; i < hdr->npoints; i++, ptr += 3)
      {
        // Due to fuzz, each point might land in more than one partition...

        for (int j = 0; j < s; j++)
        {
          if (partitioning->isIn(j, ptr, fuzz))
            recipient_vertex_count[j] ++;
        }
      }
    }
    else
    {
      // Other geometry types will have to taks connectivity into account\n";
      std::cerr << "we only handle particles at the moment\n";
      exit(1);
    }
  }

  // list of buffers containing the points to send to each server
  // begins with integer size and a header

  vector<char *> send_buffers;
  for (int i = 0; i < s; i++)
  {
    // Note - the 2* sizeof int is to add a overrun tag on the end

    int size = 2*sizeof(int) + sizeof(bufhdr)
               + recipient_vertex_count[i]*3 * sizeof(float) 
               + recipient_connectivity_count[i] * sizeof(int) 
               + (has_data ? recipient_vertex_count[i]*sizeof(float) : 0);

    char *buffer = (char *)malloc(size);

    *(int *)buffer = size;
    *(int *)(buffer + (size-4)) = 12345;

    bufhdr *sndhdr = (bufhdr *)(buffer + sizeof(int));

    sndhdr->type = btype;
    sndhdr->origin = GetTheApplication()->GetRank();
    sndhdr->has_data = has_data;
    sndhdr->npoints = recipient_vertex_count[i];
    sndhdr->nconnectivity = recipient_connectivity_count[i];
    
    send_buffers.push_back(buffer);
  }

  // next pointers for each destination points followed by data  THIS IS GOING
  // TO BE DIFFERENT IF THERE IS CONNECTIVITY DATA!

  float **dpptrs = new float*[s];
  float **ddptrs = new float*[s];

  for (int i = 0; i < s; i++)
  {
    dpptrs[i] = (float *)(send_buffers[i] + sizeof(int) + sizeof(bufhdr));
    ddptrs[i] = dpptrs[i] + 3*recipient_vertex_count[i];
  }

  // Now go through the received buffers, copying data to outgoing buffers

  for (auto buffer : buffers)
  {
    bufhdr *shdr = (bufhdr *)(buffer + sizeof(int));
    float *spptr = (float *)(shdr + 1);
    float *sdptr = spptr + 3*shdr->npoints;

    for (int i = 0; i < shdr->npoints; i++)
    {
      for (int j = 0; j < s; j++)
      {
        if (partitioning->isIn(j, spptr, fuzz))
        {
          float *dp = dpptrs[j]; dpptrs[j] = dpptrs[j] + 3;
          float *dd = ddptrs[j]; ddptrs[j] = ddptrs[j] + 1;

          *dp++ = spptr[0];
          *dp++ = spptr[1];
          *dp++ = spptr[2];
          *dd++ = *sdptr;
        }
      }

      spptr = spptr + 3;
      sdptr = sdptr + 1;
    }
  }

  // Check the overrun tags - ddptrs should point to it?
  
  for (int i = 0; i < s; i++)
    if (*(int *)ddptrs[i] != 12345)
      std::cerr << "overrun error\n";

  // Now we can delete the buffers we got from the sender(s)...

  for (auto buffer : buffers) free(buffer);
  buffers.clear();

  // Off to the races...

  lock_buffers();

  for (int i = 0; i < s; i++)
  {
    bufhdr *hdr = (bufhdr *)(send_buffers[i] + sizeof(int));
    // APP_PRINT(<< "sending " << hdr->npoints << " to " << i);

    if (i == GetTheApplication()->GetRank())
      reshuffle_buffers.push_back(send_buffers[i]);
    else
    {
      ReshuffleMsg msg(this, send_buffers[i]);
      msg.Send(i);
    }
  }

  while (reshuffle_buffers.size() < s)
    wait_buffers();

  unlock_buffers();

  int num_points = 0;

  for (auto buffer : reshuffle_buffers)
  {
    if (buffer)
    {
      bufhdr *hdr = (bufhdr *)(buffer + sizeof(int));
      num_points += hdr->npoints;
    }
  }

  geometry->RemoveTheOSPRayEquivalent();      
  geometry->allocate_vertices(num_points);    // also allocates per-vertex float data

  float *pptr = (float *)geometry->GetVertices();
  float *dptr = (float *)geometry->GetData();

  char **sorted = new char*[reshuffle_buffers.size()];
  for (auto buffer : reshuffle_buffers)
  {
    bufhdr *hdr = (bufhdr *)(buffer + sizeof(int));
    sorted[hdr->origin] = buffer;
  }

  for (int i = 0; i < reshuffle_buffers.size(); i++)
  {
    char *buffer = sorted[i];
    if (buffer)
    {
      bufhdr *hdr = (bufhdr *)(buffer + sizeof(int));
      float *vertices = (float *)(hdr + 1);
      float *data = vertices + hdr->npoints*3;
      memcpy(pptr, vertices, hdr->npoints*3*sizeof(float));
      memcpy(dptr, data, hdr->npoints*sizeof(float));
      pptr += hdr->npoints*3;
      dptr += hdr->npoints;
      free(buffer);
    }
  }

#if 0
  char namebuf[256];
  sprintf(namebuf, "RS-%d-%d.csv", GetTheApplication()->GetRank(), frame);
  ofstream ofs(namebuf);
  ofs << "X,Y,Z,D\n";
  pptr = (float *)geometry->GetVertices();
  dptr = (float *)geometry->GetData();
  for (int i = 0; i < geometry->GetNumberOfVertices(); i++)
  {
    ofs << pptr[0] << ", " << pptr[1] << ", " << pptr[2] << ", " << *dptr << "\n";
    pptr += 3;
    dptr++;
  }
  ofs.close();
#endif

  frame++;

  reshuffle_buffers.clear();
  std::cerr << "Reshuffle done\n";

  return true;
}

void
Receiver::_Receive(char *buffer)
{
  bufhdr *b = (bufhdr *)(buffer + sizeof(int));

  lock_buffers();
  
  int sz = *(int *)buffer;
  char *buf = (char *)malloc(sz);
  memcpy(buf, buffer, sz);
  reshuffle_buffers.push_back(buf);

  signal_buffers();
  unlock_buffers();

#if 0
  char fname[256];
  sprintf(fname, "%d-from-%d-%d.csv", gxy::GetTheApplication()->GetRank(), b->origin, frame);
  std::ofstream ofs(fname);
  float *pptr = (float *)(b + 1);
  float *dptr = (float *)pptr + b->npoints*3;
  ofs << "X,Y,Z,D\n";
  for (int i = 0; i < b->npoints; i++)
  {
    ofs << pptr[0] << "," << pptr[1] << "," << pptr[2] << "," << *dptr++ << "\n";
    pptr = pptr + 3;
  }
#endif

}

bool
Receiver::receiver(ServerSkt *skt, void *p, char *buf)
{
  Receiver *me = (Receiver *)p;
  return me->receive(buf);
}

bool
Receiver::receive(char *buffer)
{
  lock_state();

  if (! buffer)
  {
    state = ERROR;
    signal_state();
    unlock_state();
    return true;      // Pause socket on error
  }
  else
  {
    bufhdr *hdr = (bufhdr *)(buffer + sizeof(int));
    std::cerr << "received " << hdr->npoints << "\n";

    buffers.push_back(buffer);

    if (buffers.size() == number_of_local_attachments)
    {
      state = RECEIVE_DONE;
      signal_state();
      unlock_state();
      return true;    // Pause socket on receipt of correct number of buffers
    }
    else
    {
      unlock_state();
      return false;   // Keep on receiving
    }
  }
}

bool
Receiver::AcceptMsg::CollectiveAction(MPI_Comm c, bool isRoot)
{
  AcceptMsgArgs *p = (AcceptMsgArgs *)get();
  ReceiverP receiver = Receiver::GetByKey(p->rk);
  receiver->_Accept(c, p->nsenders);
  return false;
}

bool
Receiver::WaitMsg::CollectiveAction(MPI_Comm c, bool isRoot)
{
  WaitMsgArgs *p = (WaitMsgArgs *)get();
  ReceiverP receiver = Receiver::GetByKey(p->rk);
  return false;
}

void *
Receiver::reshuffle_thread(void *d)
{
  Receiver *me = (Receiver *)d;

  me->lock_state();

  bool done = false;
  while (! done)
  {
    while (me->state != EXITTING && me->state != START_RESHUFFLE)
      me->wait_state();

    if (me->state == EXITTING)
    {
      done = true;
    }
    else if (me->state == START_RESHUFFLE)
    {
      me->Reshuffle();

      MPI_Barrier(me->receiver_comm);

      me->state = PAUSED;
      me->signal_state();
    }
  }

  me->unlock_state();

  pthread_exit(NULL);
}


void
Receiver::_Accept(MPI_Comm c, int nsenders)
{
  lock_state();

  int status = 1;

  if (buffers.size() > 0)
  {
    std::cerr << "Got some buffers at the start of _Accept\n";
    exit(0);
  }

  int gsz, grnk;
  MPI_Comm_rank(c, &grnk);
  MPI_Comm_size(c, &gsz);

  number_of_local_attachments = (nsenders / gsz) + (((nsenders % gsz) > grnk) ? 1 : 0);

  if (number_of_local_attachments)
  {
    state = RECEIVING;

    serverskt->resume();

    while (state == RECEIVING)
      wait_state();

    status = state == RECEIVE_DONE;
  }

  // Got to wait for all receivers to receive

  int all_status;
  MPI_Allreduce(&status, &all_status, 1, MPI_INT, MPI_MIN, c);

  if (all_status)
  {
    state = START_RESHUFFLE;
    signal_state();
  }

  unlock_state();
}

bool
Receiver::HostsMsg::CollectiveAction(MPI_Comm c, bool isRoot)
{
  HostsMsgArgs *p = (HostsMsgArgs *)get();
  ReceiverP receiver = Receiver::GetByKey(p->rk);
  
  int r = GetTheApplication()->GetRank();
  int s = GetTheApplication()->GetSize();

  char *sndbuf = new char[256];

  gethostname(sndbuf, 256);
  std::stringstream ss;
  ss << sndbuf << ":" << receiver->local_port;
  
  char *rcvbuf = new char[s*256];

  MPI_Gather((void *)ss.str().c_str(), 256, MPI_CHAR, (void *)rcvbuf, 256, MPI_CHAR, 0, MPI_COMM_WORLD);

  if (r == 0)
  {
    std::stringstream ss;

    for (int i = 0; i < s; i++)
      ss << std::string(rcvbuf + i*256) << ";";

    receiver->host_list = ss.str();
  }

  delete[] sndbuf;
  delete[] rcvbuf;

  return false;
}

}
