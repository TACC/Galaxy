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

#include <Application.h>
#include <KeyedObject.h>
#include <Geometry.h>

#include "Partitioning.hpp"
#include "Skt.hpp"

namespace gxy
{

OBJECT_POINTER_TYPES(Receiver)

class Receiver : public gxy::KeyedObject
{
  KEYED_OBJECT(Receiver)

  enum receiver_state {
    NOT_RUNNING,
    RUNNING,
    EXITTING
  };

  struct commit_args
  {
    int base_port;
    Key geometry;
    Key partitioning;
    int nsenders;
  };

public:
  ~Receiver();

  virtual int serialSize();
  virtual unsigned char* serialize(unsigned char *ptr);
  virtual unsigned char* deserialize(unsigned char *ptr);

  void Start();
  void Stop();

  void SetGeometry(GeometryP g) { geometry = g; }
  void SetPartitioning(PartitioningP p) { partitioning = p; }
  void SetNSenders(int n) { nsenders = n; }
  void SetBasePort(int p) { base_port = p; }

  bool receive(char *);

protected:
  void _Stop();

private:
  GeometryP geometry = NULL;
  PartitioningP partitioning = NULL;
  int base_port = 1900;
  int nsenders = -1;

  int local_port;

  MPI_Comm receiver_comm;

  receiver_state state = NOT_RUNNING;

  pthread_t tid;
  bool thread_running = false;
  static void *reshuffle_thread(void *);

  bool exitting = false;
  bool complete = false;

  ServerSkt *serverskt = NULL;
  static bool receiver(ServerSkt*, void *, char *);

  bool Setup(MPI_Comm);
  bool Reshuffle();

  int n_total_senders;

  int number_of_local_attachments;
  int number_of_timestep_connections_received = 0;

  std::vector<char *> buffers;

  class StartMsg : public Work
  {
    struct StartMsgArgs
    {
      Key rk; // Receiver object
    };

  public:
    StartMsg(Receiver* r) : StartMsg(sizeof(Key))
    {
      struct StartMsgArgs *p = (struct StartMsgArgs *)contents->get();
      p->rk = r->getkey();
    }

    WORK_CLASS(StartMsg, true);

    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {
      struct StartMsgArgs *p = (struct StartMsgArgs *)get();
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
};

}
