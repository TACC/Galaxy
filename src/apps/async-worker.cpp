#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

using namespace std;

#include "Application.h"
#include "Renderer.h"
#include "AsyncRendering.h"

using namespace pvol;

#include <ospray/ospray.h>

class Debug
{
public:
  Debug(const char *executable, bool attach)
  {
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

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options]\n";
  cerr << "options:\n";
  cerr << "  -D         run debugger\n";
  cerr << "  -A         wait for attachment\n";
  exit(1);
}

int
main(int argc, char *argv[])
{
  bool dbg = false, atch = false;

  ospInit(&argc, (const char **)argv);

  Application theApplication(&argc, &argv);
  theApplication.Start();

  AsyncRendering::RegisterClass();

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-A")) dbg = true, atch = true;
    else if (!strcmp(argv[i], "-D")) dbg = true, atch = false;
	}

  Debug *d = dbg ? new Debug(argv[0], atch) : NULL;

  Renderer::Initialize();
  GetTheApplication()->Run();

	GetTheApplication()->Wait();

  return 0;
}
