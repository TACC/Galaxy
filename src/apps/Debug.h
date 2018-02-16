#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <sstream>

using namespace std;

class Debug
{
public:
  Debug(const char *executable, bool attach)
  {
    cerr << "Debug ctor\n";
    bool dbg = true;
    std::stringstream cmd;
    pid_t pid = getpid();

    if (attach)
      std::cerr << "Attach to PID " << pid << "\n";
    else
    {
      cmd << "~/dbg_script " << executable << " " << pid << " &";
      system(cmd.str().c_str());
    }

    while (dbg)
      sleep(1);

    std::cerr << "running\n";
  }
};
