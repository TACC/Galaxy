#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

#include <string.h>
#include <dlfcn.h>

#include "Application.h"
#include "MultiServerHandler.h"

using namespace gxy;

int mpiRank, mpiSize;

#include "Debug.h"

void
syntax(char *a)
{
  if (mpiRank == 0)
  {
    cerr << "syntax: " << a << " [options]" << endl;
    cerr << "options:" << endl;
    cerr << "  -D         run debugger" << endl;
    cerr << "  -A         wait for attachment" << endl;
    cerr << "  -P port    port to use (5001)" << endl;
  }
  MPI_Finalize();
  exit(1);
}

int 
main(int argc, char *argv[])
{
  bool dbg = false, atch = false;
  char *dbgarg = "";
  int port = 5001;

  Application theApplication(&argc, &argv);
  theApplication.Start();

  mpiRank = GetTheApplication()->GetRank();
  mpiSize = GetTheApplication()->GetSize();

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-A")) dbg = true, atch = true;
    else if (!strncmp(argv[i], "-D", 2)) dbg = true, atch = false, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i], "-P")) port = atoi(argv[++i]);
    else
      syntax(argv[0]);
  }

  Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;

  GetTheApplication()->Run();

  if (mpiRank == 0)
  {
    MultiServerP server = MultiServer::NewP(port);

    string cmd;
    for (cerr << "? ", cin >> cmd; !cin.eof() && cmd != "q" && cmd != "quit"; cerr << "? ", cin >> cmd)
    {
      if (cmd == "help")
        cout << "control-D, q or quit to quit\n";
      else 
        cerr << "huh? ";
    }

    GetTheApplication()->QuitApplication();
  }
  else
  {
    GetTheApplication()->Wait();
  }
}
