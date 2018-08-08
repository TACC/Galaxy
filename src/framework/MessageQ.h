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

/*! \file MessageQ.h 
 * \brief manages a communication queue of Messages for the MessageManager in Galaxy
 * \ingroup framework
 */

#include <deque>
#include <iostream>
#include <pthread.h>

#include "Message.h"

namespace gxy
{

//! manages a communication queue of Messages for the MessageManager in Galaxy
/*! \ingroup framework
 * \sa Message, MessageManager, Work
 */
class MessageQ {
public:
  //! constructor
  /*! \param n the name for this message queue
   */
  MessageQ(const char *n) : name(n)
  {
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&signal, NULL);
    running = true;
  }

  //! destructor
  ~MessageQ()
  {
    running = false;
    pthread_cond_broadcast(&signal);
  }

  //! stop this message queue from processing further messages
  /*! \warning after a Kill is issued, IsReady will return `1` (not ready) and 
   * Dequeue will not wait for new messages, likely returning `NULL`
   */
  void Kill();

  //! add a Message to the queue
  /*! When the message is added, this method also signals the mutex that a message has arrived.
   */
  void Enqueue(Message *w);
  //! remove a Message from the queue
  /*! If the queue is empty and the queue is running, a call to this method will block until a message is received.
   * If a Kill has been issued to this queue, this method will not block and return the next message in the queue,
   * if any, or `NULL` if no messages are present.
   */
  Message *Dequeue();
  //! is this queue ready for additional messages?
  /*! \returns `0` if the queue is empty and the queue is running, otherwise returns `1`
   */
  int IsReady();

  //! returns the number of Messages pending on this queue
	int size() { return workq.size(); }

  //! print the messages pending on this queue
	void printContents();

private:
  const char *name;

  pthread_mutex_t lock;
  pthread_cond_t signal;
  bool running;

  std::deque<Message *> workq;
};

} // namespace gxy
