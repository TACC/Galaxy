// ========================================================================== //
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
#include <string>

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

#include "SocketHandler.h"

namespace gxy
{

static void *event_thread(void *d)
{
  SocketHandler *theSocketHandler = (SocketHandler *)d;
  while(! theSocketHandler->event_thread_quit)
  {
    if(theSocketHandler->EWait(0.1))
      theSocketHandler->EventHandler();
  }
  pthread_exit(NULL);
}

SocketHandler::SocketHandler()
{
  is_connected = false;
  for (int i = 0; i < 3; i++)
    pthread_mutex_init(&locks[i], NULL);
}

SocketHandler::SocketHandler(int cfd, int dfd, int efd) : SocketHandler::SocketHandler() 
{
  fds[0] = cfd;
  fds[1] = dfd;
  fds[2] = efd;

  pthread_create(&event_tid, NULL, event_thread, (void *)this);
}

bool SocketHandler::Connect(std::string host, int port)
{
  return SocketHandler::Connect((char *)host.c_str(), port);
}

void
SocketHandler::EventHandler()
{
  char *msg = NULL; int n;
  ERecv(msg, n);
  if (msg != NULL)
    std::cerr << "Event message received: " << msg << "\n";
  free(msg);
}

bool SocketHandler::Connect(char *host, int port)
{
  if (is_connected)
  {
    std::cerr << "already connected\n";
    return true;
  }

  for (int i = 0; i < 3; i++)
    pthread_mutex_init(&locks[i], NULL);

  struct hostent *server;

  server = gethostbyname(host);
  if (server == NULL)
  {
    std::cerr <<  "ERROR: no such host (" << host << ")\n";
    return false;
  }

  struct sockaddr_in serv_addr;
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(port);

  for (int i = 0; i < 3; i++)
    if ((fds[i] = connect_fd((struct sockaddr*)&serv_addr)) < 0)
    {
      perror("ERROR opening SocketHandler socket");
      return false;
    }

  is_connected = true;

  pthread_create(&event_tid, NULL, event_thread, (void *)this);
  return true;
}

SocketHandler::~SocketHandler()
{
  if (is_connected)
  {
    event_thread_quit = true;
    pthread_join(event_tid, NULL);

    for (int i = 0; i < 3; i++)
      close(fds[i]);
  }
    
  for (int i = 0; i < 3; i++)
    pthread_mutex_destroy(&locks[i]);
}

int
SocketHandler::connect_fd(struct sockaddr* serv_addr)
{
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) return -1;

  int one = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)))
  {
    perror("setsockopt(SO_REUSEADDR) failed");
    return -1;
  }

  if (connect(fd, serv_addr, sizeof(*serv_addr)) < 0)
  {
    perror("ERROR connecting");
    return -1;
  }

  return fd;
}


static bool _receive(int fd, char *b, int n)
{
  char *bb = b;
  int nn = n;
  while(nn)
  {
    int t = read(fd, bb, nn);
    if (t <= 0)
      return false;

    bb += t;
    nn -= t;
  }

  return true;
}

static bool _send(int fd, char *b, int n)
{
  char *bb = b;
  int nn = n;
  while(nn)
  {
    int t = write(fd, bb, nn);
    if (t <= 0)
      return false;

    bb += t;
    nn -= t;
  }

  return true;
}

bool
SocketHandler::Send(int fd, const char *b, int n)
{
  if (!_send(fd, (char *)&n, sizeof(n))) return false;
  if (show) std::cout << b << "\n";
  return _send(fd, (char *)b, n);
}

bool
SocketHandler::SendV(int fd, char **b, int* n)
{
  int sz = 0;

  for (int i = 0; n[i]; i++)
    sz += n[i];

  if (! _send(fd, (char *)&sz, sizeof(sz)))
    return false;
  
  for (int i = 0; n[i]; i++)
    if (! _send(fd, b[i], n[i]))
      return false;

  return true;
}

bool
SocketHandler::Recv(int fd, char*& b, int& n)
{
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(fd, &fds);
  if (select(fd+1, &fds, NULL, NULL, NULL) < 0)
  {
    perror("select()");
    exit(1);
  }

  if (0 >= _receive(fd, (char *)&n, sizeof(n)))
  {
    n = 0;
    b = NULL;
    return false;
  }

  b = (char *)malloc(n);

  if (0 >= _receive(fd, b, n))
  {
    n = 0;
    b = NULL;
    return false;
  }

  if (show) std::cout << b << "\n";

  return true;
}

bool
SocketHandler::Wait(int fd, float sec)
{
  struct timeval tv;
  tv.tv_sec  = floor(sec);
  tv.tv_usec = (sec - tv.tv_sec) * 1000000;

  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(fd, &fds);

  return select(fd+1, &fds, NULL, NULL, &tv) != 0;
}

void
SocketHandler::Disconnect()
{
  if (is_connected)
  {
    event_thread_quit = true;
    pthread_join(event_tid, NULL);
    for (int i = 0; i < 3; i++)
      close(fds[i]);
    is_connected = false;
  }
}

}
