#pragma once

#include <pthread.h>
#include <string>

namespace gxy
{

class MultiServer
{
private:
  struct args
  {
    args(MultiServer *s, int cf, int df) : srvr(s), cfd(cf), dfd(df) {}
    MultiServer *srvr;
    int cfd, dfd;
  };

  static void* watch(void *);
  static void* start(void *);

public:
  MultiServer(int p);
  ~MultiServer();

  void Load(std::string);
  void Quit() { done = true; }

private:
  int port;
  int fd;
  bool done;
  pthread_t watch_tid;
};

}
