#pragma once

#include <pthread.h>
#include <string>

namespace gxy
{

OBJECT_POINTER_TYPES(MultiServer)

extern MultiServerP theMultiServer;

class MultiServer : public GalaxyObject
{
  GALAXY_OBJECT(MultiServer)

public:
  static MultiServerP NewP(int port)
  {
    theMultiServer = MultiServerP(new MultiServer(port));
    return theMultiServer;
  }

  static MultiServerP Get() { return theMultiServer; }

private:
  MultiServer(int p);
  static void* watch(void *);
  static void* start(void *);

public:
  ~MultiServer();

  void Quit() { done = true; }

private:
  int port;
  int fd;
  bool done;
  pthread_t watch_tid;
};

}
