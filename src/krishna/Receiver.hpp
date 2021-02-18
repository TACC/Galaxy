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

#include "Skt.hpp"

namespace gxy
{

OBJECT_POINTER_TYPES(Receiver)

class Receiver : public gxy::KeyedObject
{
  KEYED_OBJECT(Receiver)

  enum receiver_state {
    NOT_STARTED,
    RUNNING,
    EXITTING,
    FINISHED
  };

public:
  ~Receiver();

  void Start(GeometryP, int, int);
  void Stop();

  bool receive(char *);

protected:
  void _Stop();

private:
  MPI_Comm receiver_comm;

  receiver_state state = NOT_STARTED;

  pthread_t tid;
  bool thread_running = false;
  static void *reshuffle_thread(void *);

  bool exitting = false;
  bool complete = false;

  ServerSkt *serverskt = NULL;
  static bool receiver(ServerSkt*, void *, char *);

  bool Setup(MPI_Comm, GeometryP, int, int);
  bool Reshuffle();

  GeometryP geometry; // The geometry object being updated across the socket.

  int n_total_senders;

  int number_of_local_attachments;
  int number_of_timestep_connections_received = 0;

  std::vector<char *> buffers;

  class StartMsg : public Work
  {
    struct StartMsgArgs
    {
      Key rk; // Receiver object
      Key gk; // Geometry object
      int n;  // number of senders
      int bp; // base port
    };

  public:
    StartMsg(Receiver* r, GeometryP g, int n, int bp)
    : StartMsg(sizeof(struct StartMsgArgs))
    {
      struct StartMsgArgs *p = (struct StartMsgArgs *)contents->get();
      p->rk = r->getkey();
      p->gk = g->getkey();
      p->n  = n;
      p->bp = bp;
    }

    WORK_CLASS(StartMsg, true);

    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {
      int r, s;
      MPI_Comm_rank(c, &r);
      MPI_Comm_size(c, &s);

      struct StartMsgArgs *p = (struct StartMsgArgs *)get();

      ReceiverP receiver = Receiver::GetByKey(p->rk);
      GeometryP geometry = Geometry::GetByKey(p->gk);

      return receiver->Setup(c, geometry, p->n, p->bp);
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
