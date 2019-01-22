#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "Application.h"
#include "MultiServer.h"
#include "MultiServerHandler.h"

pthread_mutex_t lck = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  w8 = PTHREAD_COND_INITIALIZER;

using namespace gxy;

class PingMsg : public Work
{
public:
  PingMsg(std::string m) : PingMsg(m.size() + 1) { strcpy((char *)get(), m.c_str()); }
  ~PingMsg() {}

  WORK_CLASS(PingMsg, true)

public:
  bool Action(int s)
  {
    int rank = GetTheApplication()->GetRank();
    int size = GetTheApplication()->GetSize();

    if (rank != 0)
    {
      std::cerr << ((char *)get()) << ": " << rank << "\n";
      PingMsg m((char *)get());
      m.Send((rank == (size-1)) ? 0 : rank + 1);
    }
    else
    {
      std::cerr << "ping signalling\n";
      pthread_mutex_lock(&lck);
      pthread_cond_signal(&w8);
      pthread_mutex_unlock(&lck);
    }

    return false;
  }
};


WORK_CLASS_TYPE(PingMsg)

using namespace gxy;

extern "C" void
init()
{
  PingMsg::Register();
}

void
Ping()
{
  pthread_mutex_lock(&lck);
  PingMsg p("ping");
  p.Send(1);
  pthread_cond_wait(&w8, &lck);
  pthread_mutex_unlock(&lck);
  cerr << "ping done\n";
}

extern "C" bool
server(MultiServerHandler *handler)
{
  bool client_done = false;
  while (! client_done)
  {
    char *buf; int sz;
    if (! handler->CRecv(buf, sz))
    {
      cerr << "receive failed\n";
      break;
    }

    cerr << "received " << buf << "\n";

    if (!strcmp(buf, "ping"))
    {
      Ping();
    }
    else if (!strcmp(buf, "quit"))
    {
      client_done = true;
    }
    else
      cerr << "huh?\n";

    std::string ok("ping ok");
    handler->CSend(ok.c_str(), ok.length()+1);

    free(buf);
  }

  return true;
}
