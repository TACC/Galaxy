// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
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

#include "MessageQ.h"

#include <unistd.h>
#include <iostream>

#include "Application.h"
#include "Message.h"

using namespace std;

namespace gxy
{
void
MessageQ::Enqueue(Message *w)
{
  pthread_mutex_lock(&lock);
  workq.push_back(w);
  pthread_cond_signal(&signal);
  pthread_mutex_unlock(&lock);
}

Message *
MessageQ::Dequeue()
{
  pthread_mutex_lock(&lock);

  while (workq.empty() && running)
    pthread_cond_wait(&signal, &lock);

  Message *r = NULL;
  if (! workq.empty())
	{
    r = workq.front();
    workq.pop_front();
  }

  pthread_mutex_unlock(&lock);
  return r;
}

void
MessageQ::printContents()
{
	  for(auto a = workq.begin(); a != workq.end(); ++a)
			GetTheApplication()->Identify(*a);
}

int 
MessageQ::IsReady()
{
  pthread_mutex_lock(&lock);
  int t = (workq.empty() && running) ? 0 : 1;
  pthread_mutex_unlock(&lock);
  return t;
}

void
MessageQ::Kill()
{
  pthread_mutex_lock(&lock);
	running = false;
	pthread_cond_signal(&signal);
  pthread_mutex_unlock(&lock);
}
	
} // namespace gxy
