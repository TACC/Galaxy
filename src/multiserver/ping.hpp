#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <Application.h>

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

