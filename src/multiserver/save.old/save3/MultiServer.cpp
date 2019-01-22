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
#include "MultiServerHandler.h"

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
  MultiServerHandler *msh = (MultiServerHandler *)d;

  if (! msh->RunServer())
    std::cerr << "RunServer failed\n";

  delete msh; 

  GetTheApplication()->GetTheDynamicLibraryManager()->Flush();

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

        MultiServerHandler *msh = new MultiServerHandler(cfd, dfd);

        pthread_t tid;
        if (pthread_create(&tid, NULL, MultiServer::start, (void *)msh))
          perror("pthread_create");
      }
    }
  }

  pthread_exit(NULL);
}

MultiServerP gxy::theMultiServer = nullptr;
