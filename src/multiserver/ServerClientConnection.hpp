#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>

#include "MultiServer.h"
#include "MultiServerHandler.h"
#include "SocketHandler.h"
#include "Datasets.h"

namespace gxy
{

class ServerClientConnection : public SocketHandler
{
public:
  static void *
  thread(void *d)
  {
    ServerClientConnection *client_connection = (ServerClientConnection *)d;
    client_connection->run();
    return NULL;
  }
    
  ServerClientConnection(int cfd, int dfd, int efd) : SocketHandler(cfd, dfd, efd)
  {
    if (pthread_create(&tid, NULL, ServerClientConnection::thread, (void *)this))
      std::cerr << "error starting client handler thread";
  }

  void
  run()
  {
    std::vector<MultiServerHandler*> handlers;

    DatasetsP theDatasets = Datasets::Cast(MultiServer::Get()->GetGlobal("global datasets"));
    if (! theDatasets)
    {
      theDatasets = Datasets::NewP();
      MultiServer::Get()->SetGlobal("global datasets", theDatasets);
    }

    bool client_done = false;
    while (! client_done)
    {
      std::string line, reply;

      if (! CRecv(line))
      {
        std::cerr << "client has disconnected?\n";
        break;
      }

      std::stringstream ss(line);

      std::string cmd;
      ss >> cmd;
      
      if (cmd == "load")
      {
        std::string libname;
        ss >> libname;

        DynamicLibraryP dlp = MultiServer::Get()->getTheDynamicLibraryManager()->Load(libname);
        if (! dlp)
          reply = std::string("error loading ") + libname;
        else
        {
          MultiServerHandler::new_handler new_handler = (MultiServerHandler::new_handler)dlp->GetSym("new_handler");
          if (! new_handler)
            reply = std::string("error retrieving new_handler from ") + libname;
          else
          {
            handlers.push_back(new_handler(this));
            reply = "ok";
          }
        }
      }
      else if (cmd == "quit")
      {
        client_done = true;
        reply = "ok";
      }
      else if (cmd == "clear") {
        MultiServer::Get()->ClearGlobals();
        reply = "ok";
      }
      else
      {
        reply = std::string("no matches for ") + line;

        bool found = false;
        for (auto handler : handlers)
          if (handler->handle(line, reply))
          {
            found = true;
            break;
          }

        if (! found) 
          reply = std::string("unrecognized: ") + line;
      }

      CSend(reply.c_str(), reply.length()+1);
    }

    // for (auto it = handlers.begin(); it != handlers.end(); it++)
      // delete *it;

    for (auto handler : handlers)
      delete handler;
  }

private:
  pthread_t tid;
};

}
