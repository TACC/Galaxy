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

#include "MultiServerHandler.h"

using namespace gxy;

MultiServerHandler::MultiServerHandler(const char *host, int port)
{
  pthread_mutex_init(&c_lock, NULL);
  pthread_mutex_init(&d_lock, NULL);

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

  if ((control_fd = connect_fd((struct sockaddr*)&serv_addr)) < 0)
  {
    perror("ERROR opening control socket");
    exit(1);
  }

  if ((data_fd = connect_fd((struct sockaddr*)&serv_addr)) < 0)
  {
    perror("ERROR opening data sockets");
    exit(1);
  }
}

MultiServerHandler::~MultiServerHandler()
{
  pthread_mutex_destroy(&c_lock);
  pthread_mutex_destroy(&d_lock);
}

int
MultiServerHandler::connect_fd(struct sockaddr* serv_addr)
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
MultiServerHandler::Send(int fd, const char *b, int n)
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
MultiServerHandler::SendV(int fd, char **b, int* n)
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
MultiServerHandler::Recv(int fd, char*& b, int& n)
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
MultiServerHandler::Wait(int fd, float sec)
{
  struct timeval tv;
  tv.tv_sec  = floor(sec);
  tv.tv_usec = (sec - tv.tv_sec) * 1000000;

  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(fd, &fds);

  return select(fd+1, &fds, NULL, NULL, &tv) != 0;
}
