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

SocketHandler::SocketHandler()
{
  pthread_mutex_init(&c_lock, NULL);
  pthread_mutex_init(&d_lock, NULL);
}

SocketHandler::SocketHandler(int cfd, int dfd) : SocketHandler::SocketHandler() 
{
  data_fd = dfd;
  control_fd = cfd;
  is_connected = false;
}

bool SocketHandler::Connect(std::string host, int port)
{
  return SocketHandler::Connect((char *)host.c_str(), port);
}

bool SocketHandler::Connect(char *host, int port)
{
  if (is_connected)
  {
    std::cerr << "already connected\n";
    return true;
  }

  pthread_mutex_init(&c_lock, NULL);
  pthread_mutex_init(&d_lock, NULL);

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

  if ((control_fd = connect_fd((struct sockaddr*)&serv_addr)) < 0)
  {
    perror("ERROR opening control socket");
    return false;
  }

  if ((data_fd = connect_fd((struct sockaddr*)&serv_addr)) < 0)
  {
    perror("ERROR opening data sockets");
    return false;
  }

  is_connected = true;
  return true;
}

SocketHandler::~SocketHandler()
{
  pthread_mutex_destroy(&c_lock);
  pthread_mutex_destroy(&d_lock);
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

  if (! _receive(fd, (char *)&n, sizeof(n)))
    return false;

  b = (char *)malloc(n);

  if (! _receive(fd, b, n))
    return false;

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

}
