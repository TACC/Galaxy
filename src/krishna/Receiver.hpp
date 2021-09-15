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

#pragma once

#include <pthread.h>

#include "Application.h"
#include "KeyedObject.h"
#include "Geometry.h"

#include "Partitioning.h"
#include "Skt.hpp"

namespace gxy
{

OBJECT_POINTER_TYPES(Receiver)

class Receiver : public gxy::KeyedObject
{
  KEYED_OBJECT(Receiver)

  enum receiver_state {
    NOT_RUNNING,
    RECEIVING,
    RECEIVE_DONE,
    ERROR,
    PAUSED,
    START_RESHUFFLE,
    RESHUFFLING,
    EXITTING
  };

  struct commit_args
  {
    int base_port;
    Key geometry;
    int nsenders;
    float fuzz;
  };

  static void *reshuffle_thread(void *d);

public:
  ~Receiver();

  void initialize() override; //!< initialize this Receiver object

  virtual bool Commit() override;
  virtual bool local_commit(MPI_Comm) override;

  virtual int serialSize() override;
  virtual unsigned char* serialize(unsigned char *ptr) override;
  virtual unsigned char* deserialize(unsigned char *ptr) override;

  void Start(bool(*)(ServerSkt*, void*, char*));
  void Run();
  void Stop();
  void Accept(int n = 1);
  bool Wait();

  void SetGeometry(GeometryP g);
  void SetNSenders(int n) { nsenders = n; }
  void SetBasePort(int p) { base_port = p; }
  void SetFuzz(float f) { fuzz = f; }

  int GetNSenders() { return nsenders; }

  const char *gethosts() { return host_list.c_str(); }

protected:
  void _Stop();
  void _Receive(char *);
  bool receive(char *);

private:
  std::string host_list;

  pthread_mutex_t mutex_buffers = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t  cond_buffers = PTHREAD_COND_INITIALIZER;
  void lock_buffers() { pthread_mutex_lock(&mutex_buffers); }
  void unlock_buffers() { pthread_mutex_unlock(&mutex_buffers); }
  void signal_buffers() { pthread_cond_signal(&cond_buffers); }
  void wait_buffers() { pthread_cond_wait(&cond_buffers, &mutex_buffers); }

  pthread_mutex_t mutex_state = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t  cond_state = PTHREAD_COND_INITIALIZER;
  void lock_state() { pthread_mutex_lock(&mutex_state); }
  void unlock_state() { pthread_mutex_unlock(&mutex_state); }
  void signal_state() { pthread_cond_broadcast(&cond_state); }
  void wait_state() { pthread_cond_wait(&cond_state, &mutex_state); }

  void _Wait(MPI_Comm);
  void _Accept(MPI_Comm, int);

  int done_count;

  GeometryP geometry = NULL;
  int base_port = 1900;
  int nsenders = -1;
  float fuzz = 0;

  int local_port;

  MPI_Comm receiver_comm;

  receiver_state state = NOT_RUNNING;

  pthread_t tid;
  bool thread_running = false;

  bool exitting = false;
  bool complete = false;

  ServerSkt *masterskt = NULL;

  ServerSkt *serverskt = NULL;
  static bool receiver(ServerSkt*, void *, char *);

  bool Setup(MPI_Comm);
  bool Reshuffle();

  int n_total_senders;

  int number_of_local_attachments;
  int number_of_timestep_connections_received = 0;

  std::vector<char *> buffers;
  std::vector<char *> reshuffle_buffers;

  class RunMsg : public Work
  {
    struct RunMsgArgs
    {
      Key rk; // Receiver object
    };

  public:
    RunMsg(Receiver* r) : RunMsg(sizeof(Key))
    {
      struct RunMsgArgs *p = (struct RunMsgArgs *)contents->get();
      p->rk = r->getkey();
    }

    WORK_CLASS(RunMsg, true);

    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {
      struct RunMsgArgs *p = (struct RunMsgArgs *)get();
      ReceiverP receiver = Receiver::GetByKey(p->rk);
      return receiver->Setup(c);
    }
  };

  class StopMsg : public Work
  {
    struct StopMsgArgs
    {
      Key rk; // Receiver object
    };

  public:
    StopMsg(Receiver* r) : StopMsg(sizeof(struct StopMsgArgs))
    {
      struct StopMsgArgs *p = (struct StopMsgArgs *)contents->get();
      p->rk = r->getkey();
    }

    WORK_CLASS(StopMsg, true);

    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {
      struct StopMsgArgs *p = (struct StopMsgArgs *)get();
      ReceiverP receiver = Receiver::GetByKey(p->rk);

      receiver->_Stop();

      return false;
    }
  };

  class ReshuffleMsg : public Work
  {
    struct ReshuffleMsgArgs
    {
      Key  rk;       // Receiver object
      char buffer[]; // data
    };

  public:
    ReshuffleMsg(Receiver* r, char *buffer) :
                     ReshuffleMsg(sizeof(struct ReshuffleMsgArgs) + *(int *)buffer)
    {
      struct ReshuffleMsgArgs *p = (struct ReshuffleMsgArgs *)contents->get();
      p->rk = r->getkey();
      memcpy(p->buffer, buffer, *(int *)buffer);
    }

    WORK_CLASS(ReshuffleMsg, false);

    bool Action(int sender)
    {
      struct ReshuffleMsgArgs *p = (struct ReshuffleMsgArgs *)get();
      ReceiverP receiver = Receiver::GetByKey(p->rk);
      receiver->_Receive(p->buffer);
      return false;
    }
  };

  class AcceptMsg : public Work
  {
    struct AcceptMsgArgs
    {
      Key  rk;       // Receiver object
      int  nsenders; 
    };

  public:
    AcceptMsg(Receiver* r, int nsenders) : AcceptMsg(sizeof(struct AcceptMsgArgs))
    {
      struct AcceptMsgArgs *p = (struct AcceptMsgArgs *)contents->get();
      p->rk = r->getkey();
      p->nsenders = nsenders;
    }

    WORK_CLASS(AcceptMsg, false);

    bool CollectiveAction(MPI_Comm c, bool isRoot);
  };

  class WaitMsg : public Work
  {
    struct WaitMsgArgs
    {
      Key  rk;       // Receiver object
    };

  public:
    WaitMsg(Receiver* r) : WaitMsg(sizeof(struct WaitMsgArgs))
    {
      struct WaitMsgArgs *p = (struct WaitMsgArgs *)contents->get();
      p->rk = r->getkey();
    }

    WORK_CLASS(WaitMsg, false);

    bool CollectiveAction(MPI_Comm c, bool isRoot);
  };

  class HostsMsg : public Work
  {
    struct HostsMsgArgs
    {
      Key  rk;       // Receiver object
    };

  public:
    HostsMsg(Receiver *r) : HostsMsg(sizeof(HostsMsgArgs))
    {
      struct HostsMsgArgs *p = (struct HostsMsgArgs *)contents->get();
      p->rk = r->getkey();
    }

    WORK_CLASS(HostsMsg, false);

    bool CollectiveAction(MPI_Comm c, bool isRoot);
  };

};

}
