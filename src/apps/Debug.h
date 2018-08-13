#pragma once

#include <sstream>

class Debug
{
public:
  Debug(const char *executable, bool attach, char *arg)
  { 
    bool dbg = true;
    std::stringstream cmd;
    pid_t pid = getpid();

    bool do_me = true;
    if (*arg)
    { 
      int i; 
      for (i = 0; i < strlen(arg); i++)
        if (arg[i] == '-')
          break;
      
      if (i < strlen(arg))
      { 
        arg[i] = 0;
        int s = atoi(arg); 
        int e = atoi(arg + i + 1);
        do_me = (mpiRank >= s) && (mpiRank <= e);
      }
      else
        do_me = (mpiRank == atoi(arg));
    }

    if (do_me)
    { 
      if (attach)
        cerr << "Attach to PID " << pid << endl;
      else
      { 
        cmd << "~/dbg_script " << executable << " " << pid << " &";
        system(cmd.str().c_str());
      }
      
      while (dbg)
        sleep(1);
      
      cerr << "running" << endl;
    }
  }
};

