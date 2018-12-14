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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <dlfcn.h>

#include "Application.h"

#include "MultiServer.h"
#include "MultiServerSocket.h"

using namespace gxy;

MultiServer::MultiServer(int p) : port(p), done(false)
{
  pthread_create(&watch_tid, NULL, watch, (void *)this);
}

MultiServer::~MultiServer()
{
  done = 1;
  pthread_join(watch_tid, NULL);
}

void *
MultiServer::start(void *d)
{
  args *a = (args *)d;

  MultiServerSocket skt(a->cfd, a->dfd);

  char *buf; int sz;
  skt.CRecv(buf, sz);

  void *dl_handle = GetTheApplication()->LoadSO(buf);
  if (! dl_handle)
  {
    std::cerr << "unable to open requested worker SO: " << buf << "\n";
    pthread_exit(NULL);
  }

  typedef void (*server_function_t)(MultiServer *, MultiServerSocket *);
  server_function_t server_function = (server_function_t)dlsym(dl_handle, "server");

  if (! server_function)
  {
    std::cerr << "unable to find server in SO " << buf << ": " << dlerror() <<  "\n";
  }
  else
  {
    std::string ok("ok");
    skt.CSend(ok.c_str(), ok.length()+1);
    server_function(a->srvr, &skt);
  }

  free(buf);
  delete a;

  std::cerr << "dropping handle\n";
  dlclose(dl_handle);

}

void *
MultiServer::watch(void *d)
{
  MultiServer *me = (MultiServer *)d;
  
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
  
  if (bind(me->fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
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
      struct sockaddr_in cli_addr;
      unsigned int cli_len = sizeof(cli_addr);

      int cfd = accept(me->fd, (struct sockaddr *)&cli_addr, &cli_len);

      tv = {1, 0};

      int n = select(me->fd+1, &fds, NULL, NULL, &tv);
      if (n <= 0)
      {
        std::cerr << "connection failed\n";
      }
      else
      {
        int dfd = accept(me->fd, (struct sockaddr *)&cli_addr, &cli_len);

        args *a = new args(me, cfd, dfd);

        pthread_t tid;
        if (pthread_create(&tid, NULL, MultiServer::start, (void *)a))
          perror("pthread_create");
      }
    }
  }

  pthread_exit(NULL);
}
