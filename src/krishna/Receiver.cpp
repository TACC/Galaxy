
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
#include "bufhdr.h"

using namespace gxy;

#include <stdio.h>
#include <stdlib.h>

namespace gxy {

WORK_CLASS_TYPE(Receiver::StartMsg)
WORK_CLASS_TYPE(Receiver::StopMsg)
WORK_CLASS_TYPE(Receiver::ReshuffleMsg)
KEYED_OBJECT_CLASS_TYPE(Receiver)

void
Receiver::Register()
{
  RegisterClass();
  Receiver::StartMsg::Register();
  Receiver::StopMsg::Register();
  Receiver::ReshuffleMsg::Register();
}

Receiver::~Receiver()
{
  if (state == RUNNING)
    _Stop();


  // std::cerr << "Freeing receiver comm\n";
  MPI_Comm_free(&receiver_comm);
  // std::cerr << "receiver comm freed\n";
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
  a->partitioning = partitioning ? partitioning->getkey() : -1;
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
  partitioning = a->partitioning != -1 ? Partitioning::GetByKey(a->partitioning) : NULL;
  nsenders = a->nsenders;
  fuzz = a->fuzz;

  return ptr + sizeof(commit_args);
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

    serverskt->stop();

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

    serverskt = NULL;
  }
  else
  {
    // If this process was not hosting a server, its running a thread to 
    // join in shuffle.   Its either in Allreduce or shuffling and wull
    // exit as soon as an Allreduce returns a global state of 0

    pthread_join(tid, NULL);
  }
}

void *
Receiver::reshuffle_thread(void *d)
{
  std::cerr << "res=huffle thread start\n";

  Receiver *rcvr = (Receiver *)d;
  bool done = false;

  while (! done)
  {
    int status = 1;
    int all_status;
    std::cerr << "Receiver::reshuffle_thread calling Allreduce\n";
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
Receiver::Start()
{
  StartMsg msg(this);
  msg.Broadcast(true, true);
}

bool
Receiver::Setup(MPI_Comm comm)
{
  state = RUNNING;

  int gsz, grnk;
  MPI_Comm_rank(comm, &grnk);
  MPI_Comm_size(comm, &gsz);

  MPI_Comm_dup(comm, &receiver_comm);

  local_port = base_port + grnk;

  number_of_local_attachments = (nsenders / gsz) + (((nsenders % gsz) > grnk) ? 1 : 0);

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
    serverskt = new ServerSkt(local_port, receiver, this);
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
  std::cerr << r << " reshuffle\n";

  // First we see how much data we need to send to each recipient

  int *recipient_vertex_count = new int[partitioning->number_of_partitions()];
  memset(recipient_vertex_count, 0, partitioning->number_of_partitions()*sizeof(int));

  int *recipient_connectivity_count = new int[partitioning->number_of_partitions()];
  memset(recipient_connectivity_count, 0, partitioning->number_of_partitions()*sizeof(int));

  // For each input buffer we got from the socket...

  bufhdr::btype btype = bufhdr::None;
  bool has_data;

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

      for (int i = 0; i < hdr->npoints; i++, ptr += 3)
      {
        // Due to fuzz, each point might land in more than one partition...

        for (int j = 0; j < partitioning->number_of_partitions(); j++)
        {
          if (partitioning->IsIn(j, ptr, fuzz))
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
  for (int i = 0; i < partitioning->number_of_partitions(); i++)
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

  float **dpptrs = new float*[partitioning->number_of_partitions()];
  float **ddptrs = new float*[partitioning->number_of_partitions()];

  for (int i = 0; i < partitioning->number_of_partitions(); i++)
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
      for (int j = 0; j < partitioning->number_of_partitions(); j++)
      {
        if (partitioning->IsIn(j, spptr, fuzz))
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
  
  for (int i = 0; i < partitioning->number_of_partitions(); i++)
    if (*(int *)ddptrs[i] != 12345)
      std::cerr << "overrun error 1\n";

  // Now we can delete the buffers we got from the sender(s)...

  for (auto buffer : buffers) free(buffer);
  buffers.clear();

  // Off to the races...

  pthread_mutex_lock(&lock);

  for (int i = 0; i < partitioning->number_of_partitions(); i++)
  {
    if (i == GetTheApplication()->GetRank())
      reshuffle_buffers.push_back(send_buffers[i]);
    else
    {
      // std::cerr << GetTheApplication()->GetRank() << " sendto " << i <<"\n";
      ReshuffleMsg msg(this, send_buffers[i]);
      msg.Send(i);
    }
  }

  while (reshuffle_buffers.size() < (partitioning->number_of_partitions()-1))
  {
    // std::cerr << GetTheApplication()->GetRank() << " bsz " << reshuffle_buffers.size() << "\n";
    pthread_cond_wait(&cond, &lock);
  }

  pthread_mutex_unlock(&lock);

  int num_points = 0;

  for (auto buffer : reshuffle_buffers)
  {
    if (buffer)
    {
      bufhdr *hdr = (bufhdr *)(buffer + sizeof(int));
      num_points += hdr->npoints;
    }
  }

  geometry->allocate_vertices(num_points);    // also allocates per-vertex float data

  float *pptr = (float *)geometry->GetVertices();
  float *dptr = (float *)geometry->GetData();

  for (auto buffer : reshuffle_buffers)
  {
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

  reshuffle_buffers.clear();

  return true;
}

void
Receiver::_Receive(char *buffer)
{
  pthread_mutex_lock(&lock);
  
  int sz = *(int *)buffer;
  char *buf = (char *)malloc(sz);
  memcpy(buf, buffer, sz);
  reshuffle_buffers.push_back(buf);

  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&lock);
}

// Called from the socket handler with data from the sender

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
      std::cerr << "Receiver::receive calling Allreduce\n";
      MPI_Allreduce(&status, &all_status, 1, MPI_INT, MPI_MIN, receiver_comm);

      state = NOT_RUNNING;
    }

    return true; // we're done - want socket thread loop to quit
  }
  else
  {
    // If the receiver is locked, then its in the process of
    // cleaning up and we DO NOT want to enter the Allreduce

    bufhdr *hdr = (bufhdr *)(buffer + sizeof(int));
    // std::cerr << GetTheApplication()->GetRank() << " received from sender " << hdr->origin << "\n";

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
        state = NOT_RUNNING;
        return true; // we're done - want socket thread loop to quit
      }
    }
  }

  return false;  // socket thread needs to keep going
}


}
