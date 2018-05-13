#include <string>
#include <unistd.h>
#include <sstream>
#include <time.h>

#include "Application.h"
#include "Renderer.h"
#include "ClientServer.h"

using namespace pvol;

#include <ospray/ospray.h>

int mpiRank, mpiSize;

void
syntax(char *a)
{
  std::cerr << "syntax: " << a << " [options] json\n";
  std::cerr << "optons:\n";
  std::cerr << "  -C         put output in Cinema DB (no)\n";
  std::cerr << "  -D         run debugger\n";
  std::cerr << "  -A         wait for attachment\n";
  std::cerr << "  -s w h     window width, height (256 256)\n";
  std::cerr << "  -S k       render only every k'th rendering\n";
  std::cerr << "  -c         client/server interface\n";
  exit(1);
}

long 
my_time()
{
  timespec s;
  clock_gettime(CLOCK_REALTIME, &s);
  return 1000000000*s.tv_sec + s.tv_nsec;
}

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
  }
};

int main(int argc,  char *argv[])
{
  string statefile("");
	char *dbgarg;
  bool dbg = false;
  bool atch = false;
  bool cinema = false;
  int width = 256, height = 256;
  int skip = 0;
  bool clientserver = false;
  ClientServer cs;

  ospInit(&argc, (const char **)argv);

  Application theApplication(&argc, &argv);
  theApplication.Start();

	for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-A")) dbg = true, atch = true, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i], "-C")) cinema = true;
    else if (!strcmp(argv[i], "-c")) clientserver = true;
    // else if (!strcmp(argv[i], "-D")) dbg = true, atch = false, dbgarg = argv[i] + 2;
    else if ((argv[i][0] == '-') && (argv[i][1] == 'D')) dbg = true, atch = false, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i], "-s")) width = atoi(argv[++i]), height = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-S")) skip = atoi(argv[++i]);
    else if (statefile == "")   statefile = argv[i];
    else syntax(argv[0]);
  }

  if (statefile == "")
    syntax(argv[0]);

  Renderer::Initialize();

  theApplication.Run();

  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;

  if (mpiRank == 0)
  {

    if (clientserver)
    {
      char hn[256];
      gethostname(hn, 256);
      std::cerr << "root is on host: " << hn << "\n";
      cs.setup_server();
      std::cerr << "connection ok\n";
    }

		long t_run_start = my_time();

    RendererP theRenderer = Renderer::NewP();

    Document *doc = GetTheApplication()->OpenInputState(statefile);

#if 0
    for (Value::ConstMemberIterator itr = doc->MemberBegin(); itr != doc->MemberEnd(); ++itr)
      printf("member %s\n", itr->name.GetString());
#endif

    theRenderer->LoadStateFromDocument(*doc);
    // theRenderer->Commit();

    vector<CameraP> theCameras = Camera::LoadCamerasFromJSON(*doc);
    for (auto c : theCameras)
      c->Commit();

    DatasetsP theDatasets = Datasets::NewP();
    theDatasets->LoadFromJSON(*doc);
    theDatasets->Commit();

    vector<VisualizationP> theVisualizations = Visualization::LoadVisualizationsFromJSON(*doc);
    for (auto v : theVisualizations)
    {
        v->Commit(theDatasets);
    }


    vector<RenderingSetP> theRenderingSets;

    RenderingSetP rs = RenderingSet::NewP();
    theRenderingSets.push_back(rs);

    int index = 0, k = 0;
    for (auto c : theCameras)
        for (auto v : theVisualizations)
        {
            if (skip && (k++ % skip) != 0)
              continue;

            RenderingP theRendering = Rendering::NewP();
            theRendering->SetTheOwner(index++ % mpiSize );
            theRendering->SetTheSize(width, height);
            theRendering->SetTheCamera(c);
            theRendering->SetTheDatasets(theDatasets);
            theRendering->SetTheVisualization(v);
            theRendering->Commit();
            rs->AddRendering(theRendering);
        }

    std::cout << "index = " << index << "\n";

    rs->Commit();

    theApplication.SyncApplication();

    for (auto rs : theRenderingSets)
    {
      std::cout << "render start\n";
      long t_rendering_start = my_time();
      theRenderer->Render(rs);

#if 1
#ifdef PRODUCE_STATUS_MESSAGES
      while (rs->Busy())
      {
        rs->DumpState();
        sleep(1);
      }
#else
      if (clientserver)
      {
        std::cerr << "Renderer running\n";

        char c;
        while (read(cs.get_socket(), &c, 1) > 0)
        {
#ifdef PRODUCE_STATUS_MESSAGES
          if (c == 's') std::cerr << "got s\n", rs->DumpState();
#endif
          if (c == 'd') std::cerr << "got d\n", theApplication.DumpEvents();
          if (c == 'q') break;
        }
      }
#endif
#endif

      rs->WaitForDone();

      rs->SaveImages(cinema ? "cinema.cdb/image/image" : "image");

      long t_rendering_end = my_time();

      std::cout << "render prep " << (t_rendering_start - t_run_start) / 1000000000.0 << " seconds\n";
      std::cout << "renderend " << (t_rendering_end - t_rendering_start) / 1000000000.0 << " seconds\n";
    }

    theApplication.QuitApplication();
  }

  theApplication.Wait();
}
