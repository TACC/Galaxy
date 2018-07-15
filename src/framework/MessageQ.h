// ========================================================================== //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 //
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

#include <deque>
#include <iostream>
#include <pthread.h>

#include "Message.h"

namespace gxy
{

class MessageQ {
public:
  MessageQ(const char *n) : name(n)
  {
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&signal, NULL);
    running = true;
  }

  ~MessageQ()
  {
    running = false;
    pthread_cond_broadcast(&signal);
  }

  void Kill();

  void Enqueue(Message *w);
  Message *Dequeue();
  int IsReady();

	int size() { return workq.size(); }

	void printContents();

private:
  const char *name;

  pthread_mutex_t lock;
  pthread_cond_t signal;
  bool running;

  std::deque<Message *> workq;
};

MessageQ *GetIncomingMessageQueue();
MessageQ *GetOutgoingMessageQueue();

} // namespace gxy
