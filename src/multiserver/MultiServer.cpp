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

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <dlfcn.h>

using namespace std;

#include "MultiServer.h"
#include "ServerClientConnection.hpp"

namespace gxy
{

static MultiServer* theMultiServer = NULL;

MultiServer* MultiServer::Get() { return theMultiServer; }

void MultiServer::Register()
{
  DynamicLibrary::Register();
  DynamicLibraryManager::Register();
}

MultiServer::MultiServer()
{
  Register();
  theMultiServer = this;
  dynamicLibraryManager = new DynamicLibraryManager;
}

MultiServer::~MultiServer()
{
  done = 1;
  ClearGlobals();
  delete dynamicLibraryManager;
  pthread_join(watch_tid, NULL);
}

void
MultiServer::Start(int p)
{
  port = p;
  done = false;
  pthread_create(&watch_tid, NULL, watch, (void *)this);
}

void *
MultiServer::watch(void *d)
{
  MultiServer *me = (MultiServer *)d;
  std::vector<pthread_t> connection_threads;
  
  me->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (me->fd < 0)
  {
   perror("ERROR opening socket");
   exit(1);
  }
  
  struct sockaddr_in serv_addr;

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(me->port);
  
  if (::bind(me->fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
  { 
    perror("ERROR on binding");
    exit(1);
  }
  
  listen(me->fd, 5);

  while (! me->done)
  {
    struct timeval tv = {1, 0};

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(me->fd, &fds);

    int n = select(me->fd+1, &fds, NULL, NULL, &tv);
    if (n > 0)
    {
      string reply = "ok";

      struct sockaddr_in cli_addr;
      unsigned int cli_len = sizeof(cli_addr);

      int dfd, cfd = accept(me->fd, (struct sockaddr *)&cli_addr, &cli_len);

      tv = {1, 0};

      int n = select(me->fd+1, &fds, NULL, NULL, &tv);
      if (n <= 0)
        reply = "connection failed";

      dfd = accept(me->fd, (struct sockaddr *)&cli_addr, &cli_len);

      ServerClientConnection *scc = new ServerClientConnection(cfd, dfd);

      // pthread_t tid;
      // if (pthread_create(&tid, NULL, ServerClientConnection::thread, (void *)scc))
      // reply = "error starting client handler thread";
      // 
      // connection_threads.push_back(tid);
      // cerr << reply << "\n";
    }
  }

  pthread_exit(NULL);
}

void
MultiServer::DropGlobal(string name)
{
  KeyedObjectP kop = globals[name];
  if (kop)
    globals.erase(name);
}

string
MultiServer::GetGlobalNames()
{
  string s = "";
  for (auto i : globals)
    s = s + i.first + ";";
  return s;
}

}
