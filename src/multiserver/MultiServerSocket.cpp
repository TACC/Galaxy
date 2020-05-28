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
#include <sys/ioctl.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>

#include "MultiServerSocket.h"

using namespace gxy;

MultiServerSocket::MultiServerSocket(const char *host, int port)
{
  for (int i = 0; i < 3; i++)
    pthread_mutex_init(&locks[i], NULL);

  struct hostent *server;

  server = gethostbyname(host);
  if (server == NULL)
  {
    std::cerr <<  "ERROR: no such host (" << host << ")\n";
    exit(0);
  }

  struct sockaddr_in serv_addr;
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(port);

  for (int i = 0; i < 3; i++)
  {
    fds[i] = connect_fd((struct sockaddr*)&serv_addr)) < 0)
    if (fds[i] < 0)
    {
      perror("ERROR opening multiserver socket");
      exit(1);
    }
  }
}

MultiServerSocket::~MultiServerSocket()
{
  for (int i = 0; i < 3; i++)
    pthread_mutex_destroy(&locks[i]);
}

int
MultiServerSocket::connect_fd(struct sockaddr* serv_addr)
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

bool
MultiServerSocket::Send(int fd, const char *b, int n)
{
  if (sizeof(n) > write(fd, &n, sizeof(n)))
  {
    return false;
  }

  while(n)
  {
    int t = write(fd, b, n);
    if (t <= 0)
      return false;

    n -= t;
    b += t;
  }

  return true;
}

bool
MultiServerSocket::SendV(int fd, char **b, int* n)
{
  int sz = 0;

  for (int i = 0; n[i]; i++)
    sz += n[i];

  write(fd, &sz, sizeof(sz));
  for (int i = 0; n[i]; i++)
  {
    int nn = n[i];
    char *bb = b[i];
    while(nn)
    {
      int t = write(fd, bb, nn);
      if (t <= 0)
      {
        return false;
      }
      nn -= t;
      bb += t;
    }
  }

  return true;
}

bool
MultiServerSocket::Recv(int fd, char*& b, int& n)
{
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(fd, &fds);
  if (select(fd+1, &fds, NULL, NULL, NULL) < 0)
  {
    perror("select()");
    exit(1);
  }

  if (sizeof(n) > read(fd, &n, sizeof(n))) 
    return false;

  b = (char *)malloc(n);
  char *bb = b;
  int nn = n;
  while (nn)
  {
    int t = read(fd, bb, nn);
    if (t <= 0)
      return false;

    bb += t;
    nn -= t;
  }

  return true;
}

bool
MultiServerSocket::Wait(int fd, float sec)
{
  struct timeval tv;
  tv.tv_sec  = floor(sec);
  tv.tv_usec = (sec - tv.tv_sec) * 1000000;

  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(fd, &fds);

  return select(fd+1, &fds, NULL, NULL, &tv) != 0;
}
