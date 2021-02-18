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

#include "Receiver.hpp"

using namespace gxy;

#include <stdio.h>
#include <stdlib.h>

namespace gxy {

WORK_CLASS_TYPE(Receiver::StartMsg)
WORK_CLASS_TYPE(Receiver::StopMsg)
KEYED_OBJECT_CLASS_TYPE(Receiver)

void
Receiver::Register()
{
  RegisterClass();
  Receiver::StartMsg::Register();
  Receiver::StopMsg::Register();
}

Receiver::~Receiver()
{
}

void
Receiver::Stop()
{
  StopMsg msg(this);
  msg.Broadcast(true, true);
}

void
Receiver::_Stop()
{
  // Got to clean up threads - *all* 
  // processes have threads handling shuffling of the data.  Some 
  // may already be blocked in the Allreduce; we need to make sure
  // that any process not already in Allreduce calls it exactly one 
  // more time so everyone can exit.

  // The rule is that Allreduce will always be called again whenever the
  // prior Allreduce returns status 1.

  state = EXITTING;

  // If this process is handling a socket for input...

  if (serverskt)
  {
    // Set a flag so that the socket thread quits before it checks
    // for new data.

    serverskt->set_done();

    // At the moment that this procedure (running in a main MPI thread 
    // collective) sets the socket done and exitting flags, the socket
    // thread loop will be in one of several states.  We need to have
    // the socket thread loop return from Allreduce *exactly once*
    // regardless of which of these states its in.

    // 1.  The server is *waiting for* input.  The socket thread will
    // break and call the receiver procedure *one last time* with a NULL
    // pointer. Since the 'finished' flag is *not* set, the receiver
    // procedure will enter Allreduce with a status of local status of 0,
    // since the buffer pointer was 0. The socket thread will then set
    // the 'finished' flag and terminate, ensuring that Allreduce will
    // not be called again.

    // 2.  The server is *in the process of receiving data*.  It will 
    // call the receiver procedure when the data has fully arrived (or
    // its been interrupted). The receiver procedure will enter Allreduce
    // with a local status of 1 (since as far as it knows) things are 
    // copacetic. One of two cases here: the returned global state is
    // 1 and 0.

    //   2a. Global state is 0.   Fine.  We just set finished and return.
    //   The socket loop will break and call the receiver process one
    //   more time with a NULL pointer.   Since the finished flag is set,
    //   we do nothing.  The thread will exit without calling Allreduce.

    //   2b.  Global state is 1.  Also fine.   Proceed with shuffle, then
    //   return to the socket thread loop.   This time this process's 
    //   loop will exit, calling the receiver process one last time with
    //   a NULL pointer.   Since the finished flag is *not* set, it 
    //   enters the Allreduce with a status of 0.   When this completes,
    //   it returns, and the thread exits.

    // 3.  This process' receiver procedure is already hung in Allreduce.
    // It will exit this loop eventually, due to events in other processes.
    // This devolves to cases 2a and 2b.

    // 4.  Shuffle is underway.  When the shuffle completes, it returns
    // to the socket handler loop, which exits and we are in the equivalent
    // of case 1.

    serverskt->wait();
    delete serverskt;
  }
  else
  {
    // If this process was not hosting a server, its running a thread to 
    // join in shuffle.   Its either in Allreduce or shuffling and wull
    // exit as soon as an Allreduce returns a global state of 0

    pthread_join(tid, NULL);
  }

  MPI_Comm_free(&receiver_comm);
}

void *
Receiver::reshuffle_thread(void *d)
{
  Receiver *rcvr = (Receiver *)d;
  bool done = false;

  while (! done)
  {
    int status = 1;
    int all_status;
    MPI_Allreduce(&status, &all_status, 1, MPI_INT, MPI_MIN, rcvr->receiver_comm);

    if (all_status)
    {
      rcvr->Reshuffle();
      MPI_Barrier(rcvr->receiver_comm);
    }
    else
    {
      // The Allreduce was completed by someone who knew to exit.
      // This is because at least one process was *not* blocked in
      // Allreduce - otherwise, the Allreduce would have completed.
      // Those processes will call Allreduce in the dtor, sending in
      // a status of 0, and the Allreduces will all complete with
      // all_status 0.

      done = true;
    }
  }

  pthread_exit(NULL);
}

void
Receiver::Start(GeometryP g, int n_senders, int base_port)
{
  StartMsg msg(this, g, n_senders, base_port);
  msg.Broadcast(true, true);
}

bool
Receiver::Setup(MPI_Comm comm, GeometryP g, int n, int bp)
{
  state = RUNNING;

  int r, s;
  MPI_Comm_rank(comm, &r);
  MPI_Comm_size(comm, &s);

  MPI_Comm_dup(comm, &receiver_comm);

  int port = bp + r;
  n_total_senders = n;
  number_of_local_attachments = (n / s) + (((n % s) > r) ? 1 : 0);

  // If there are connections expected, start a server to receive them.
  // This will start a thread that expresses a port for connections.
  // When the correct number of connections have been received for a 
  // timestep, that thread will to wait for everyone to be 
  // ready, then enter Reshuffle.
  //
  // Otherwise, there are no connections expected and we need to create
  // our *own* thread to participate in the reshuffle.

  if (number_of_local_attachments)
  {
    serverskt = new ServerSkt(port, receiver, this);
    serverskt->start();
  }
  else
  {
    pthread_create(&tid, NULL, reshuffle_thread, this);
  }

  return false;
}

bool
Receiver::Reshuffle()
{
  int r = GetTheApplication()->GetRank();

  for (auto buffer : buffers)
  {
    int *ptr = (int *)buffer;
    if (!ptr)
      std::cerr << "ptr is NULL\n";
    else
    {
      std::cerr << ptr[0] << "::" << ptr[1] << "\n";
      free(buffer);
    }
  }

  buffers.clear();

  return true;
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
  if (! buffer)
  {
    if (state == EXITTING)
    {
      int status = 0;
      int all_status;
      MPI_Allreduce(&status, &all_status, 1, MPI_INT, MPI_MIN, receiver_comm);

      state = FINISHED;
    }

    return true; // we're done - want socket thread loop to quit
  }
  else
  {
    // If the receiver is locked, then its in the process of
    // cleaning up and we DO NOT want to enter the Allreduce

    buffers.push_back(buffer);
    number_of_timestep_connections_received ++;

    if (number_of_timestep_connections_received == number_of_local_attachments)
    {
      int status = 1;
      int all_status;
      MPI_Allreduce(&status, &all_status, 1, MPI_INT, MPI_MIN, receiver_comm);

      if (all_status)
        Reshuffle();

      MPI_Barrier(receiver_comm);
      number_of_timestep_connections_received = 0;

      if (! all_status)
      {
        state = FINISHED;
        return true; // we're done - want socket thread loop to quit
      }
    }
  }

  return false;  // socket thread needs to keep going
}

}
