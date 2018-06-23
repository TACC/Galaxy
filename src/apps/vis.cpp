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
  std::cerr << "  -C cdb     put output in Cinema DB (no)\n";
  std::cerr << "  -D         run debugger\n";
  std::cerr << "  -A         wait for attachment\n";
  std::cerr << "  -s w h     window width, height (1920 1080)\n";
  std::cerr << "  -S k       render only every k'th rendering\n";
  std::cerr << "  -c         client/server interface\n";
  std::cerr << "  -N         max number of simultaneous renderings (VERY large)\n";
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
  string  cdb("");
  char *dbgarg;
  bool dbg = false;
  bool atch = false;
  bool cinema = false;
  int width = 1920, height = 1080;
  int skip = 0;
  bool clientserver = false;
  ClientServer cs;
  int maxConcurrentRenderings = 99999999;

  ospInit(&argc, (const char **)argv);

  Application theApplication(&argc, &argv);
  theApplication.Start();

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-A")) dbg = true, atch = true, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i], "-C")) cinema = true, cdb = argv[++i];
    else if (!strcmp(argv[i], "-c")) clientserver = true;
    else if ((argv[i][0] == '-') && (argv[i][1] == 'D')) dbg = true, atch = false, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i], "-s")) width = atoi(argv[++i]), height = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-S")) skip = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-N")) maxConcurrentRenderings = atoi(argv[++i]);
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

    theRenderer->LoadStateFromDocument(*doc);

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
            if (skip && (k % skip) != 0)
              continue;

            if (rs->GetNumberOfRenderings() >= maxConcurrentRenderings)
            {
              rs = RenderingSet::NewP();
              theRenderingSets.push_back(rs);
            }

            RenderingP theRendering = Rendering::NewP();
            theRendering->SetTheOwner(index++ % mpiSize );
            theRendering->SetTheSize(width, height);
            theRendering->SetTheCamera(c);
            theRendering->SetTheDatasets(theDatasets);
            theRendering->SetTheVisualization(v);
            theRendering->Commit();
            rs->AddRendering(theRendering);

            k ++;
        }

    std::cout << "index = " << index << "\n";

    rs->Commit();

    theApplication.SyncApplication();

		long t_rendering_start = my_time();

    for (auto rs : theRenderingSets)
    {
			long t0 = my_time();
      std::cout << "render start\n";

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

      rs->SaveImages(cinema ? (cdb + "/image/image").c_str() : "image");

			long t1 = my_time();
			std::cout << rs->GetNumberOfRenderings() << ": " << ((t1 - t0) / 1000000000.0) << " seconds\n";
    }

		std::cout << "TIMING rendering " << (my_time() - t_rendering_start) / 1000000000.0 << " seconds\n";

    theApplication.QuitApplication();
  }

  theApplication.Wait();
}
