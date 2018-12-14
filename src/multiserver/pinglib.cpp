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

#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

#include <string.h>
#include <pthread.h>

#include "Application.h"
#include "MultiServer.h"
#include "MultiServerSocket.h"

pthread_mutex_t lck = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  w8 = PTHREAD_COND_INITIALIZER;

#include "ping.hpp"

WORK_CLASS_TYPE(PingMsg)

using namespace gxy;

extern "C" void
init()
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

extern "C" bool
server(MultiServer *srvr, MultiServerSocket *skt)
{
  bool client_done = false;
  while (! client_done)
  {
    char *buf; int sz;
    if (! skt->CRecv(buf, sz))
    {
      cerr << "receive failed\n";
      break;
    }

    cerr << "received " << buf << "\n";

    if (!strcmp(buf, "ping"))
    {
      Ping();
      std::string ok("ping ok");
      skt->CSend(ok.c_str(), ok.length()+1);
    }
    else if (!strcmp(buf, "quit"))
    {
      client_done = true;
      std::string ok("quit ok");
      skt->CSend(ok.c_str(), ok.length()+1);
    }
    else
      cerr << "huh?\n";

    free(buf);
  }

  pthread_exit(NULL);
}
