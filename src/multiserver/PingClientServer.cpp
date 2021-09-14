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

#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "PingClientServer.h"

namespace gxy
{

extern "C" void
init()
{
  PingClientServer::init();
}

extern "C" MultiServerHandler *
new_handler(SocketHandler *sh)
{
  PingClientServer *pcs = new PingClientServer;
  pcs->SetSocketHandler(sh);
  return pcs;
}

static pthread_mutex_t lck = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  w8 = PTHREAD_COND_INITIALIZER;

bool 
PingMsg::Action(int s)
{
  int rank = GetTheApplication()->GetRank();
  int size = GetTheApplication()->GetSize();

  if (rank != 0)
  {
    std::cerr << ((char *)get()) << ": " << rank << "\n";
    PingMsg m((char *)get());
    m.Send((rank == (size-1)) ? 0 : rank + 1);
  }
  else
  {
    std::cerr << "ping signalling\n";
    pthread_mutex_lock(&lck);
    pthread_cond_signal(&w8);
    pthread_mutex_unlock(&lck);
  }

  return false;
}

WORK_CLASS_TYPE(PingMsg)

void
PingClientServer::init()
{
  PingMsg::Register();
}

void
Ping()
{
  pthread_mutex_lock(&lck);
  PingMsg p("ping");
  p.Send(1);
  pthread_cond_wait(&w8, &lck);
  pthread_mutex_unlock(&lck);
  cerr << "ping done\n";
}

bool
PingClientServer::handle(std::string line, std::string& reply)
{
  if (line == "ping")
  {
    Ping();
    reply = std::string("ok");
    return true;
  }
  else
    return false;
}


}
