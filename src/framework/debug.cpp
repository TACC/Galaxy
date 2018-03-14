#include <string>
#include "Application.h"

using namespace std;

namespace pvol
{
static string executable;

void setup_debugger(char *e)
{
	executable = e;
}

static bool debugger_running = false;

void debugger(char *arg)
{
	if (arg == NULL)
		return;

  int rank = GetTheApplication()->GetRank();

  auto c = arg;
  bool done = false, dbg = *c == 0;
  while (!done && !dbg)
  {
    auto d = c;
    while (*d && *d != ',') d++;
    done = (*d == 0);
		auto r = atoi(c);
		dbg = ((r == -1) || (r == rank));
    c = d + 1;
  }

  if (dbg)
	{
		if (! debugger_running)
		{
			std::stringstream cmd;
			pid_t pid = GetTheApplication()->get_pid();

			cmd << "~/dbg_script " << executable << " " << pid << " " << GetTheApplication()->GetRank() << " &";
			std::cerr << "running command: " << cmd.str() << "\n";
			system(cmd.str().c_str());
		}

		int dbg = 1;
		while (dbg)
			sleep(1);
	}
}

}