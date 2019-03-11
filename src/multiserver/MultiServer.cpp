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
#include "MultiServerHandler.h"

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
MultiServer::start(void *d)
{
  MultiServerHandler *msh = (MultiServerHandler *)d;

  if (! msh->RunServer())
    cerr << "RunServer failed\n";

  delete msh;

  MultiServer::Get()->getTheDynamicLibraryManager()->Flush();

  pthread_exit(NULL);
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

      char *libname;

      if (reply == "ok")
      {
        dfd = accept(me->fd, (struct sockaddr *)&cli_addr, &cli_len);

        int sz;
        char *bb = (char *)&sz;
        int nn = sizeof(sz);
        while(nn && reply == "ok")
        {
          int t = read(cfd, bb, nn);
          if (t <= 0)
            reply = "error reading libname";

          bb += t;
          nn -= t;
        }

        libname = (char *)malloc(sz);

        bb = libname;
        nn = sz;
        while(nn && reply == "ok")
        {
          int t = read(cfd, bb, nn);
          if (t <= 0)
            reply = "error reading libname\n";

          bb += t;
          nn -= t;
        }
      }

      MultiServerHandler *msh = NULL;

      if (reply == "ok")
      {
        DynamicLibraryP dlp = MultiServer::Get()->getTheDynamicLibraryManager()->Load(libname);
        free(libname);

        if (! dlp)
          reply = string("error loading ") + libname;

        if (reply == "ok")
        {
          MultiServerHandler::new_handler nh = (MultiServerHandler::new_handler)dlp->GetSym("new_handler");
          if (! nh)
            reply = string("error retrieving new_handler from ") + libname;

          if (reply == "ok")
          {
            msh = nh(dlp, cfd, dfd);
            dlp = NULL;
            if (! nh)
              reply = string("error new_handler(") + libname + ") returned NULL";
          }
        }
      }

      int sz = reply.size() + 1;
      char *bb = (char *)&sz;
      int nn = sizeof(sz);
      while(nn)
      {
        int t = write(cfd, bb, nn);
        if (t <= 0)
        {
          std::cerr << "error sending reply\n";
          break;
        }

        bb += t;
        nn -= t;
      }

      bb = (char *)reply.c_str();
      nn = reply.size() + 1;
      while(nn)
      {
        int t = write(cfd, bb, nn);
        if (t <= 0)
        {
          std::cerr << "error sending reply\n";
          break;
        }

        bb += t;
        nn -= t;
      }

      if (reply == "ok")
      {
        pthread_t tid;
        if (pthread_create(&tid, NULL, MultiServer::start, (void *)msh))
          reply = "error starting client handler thread";
      };

      cerr << reply << "\n";
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
