#include <iostream>

#include <thread>
#include <stdlib.h>
#include <stdio.h>
#include <chrono>

#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

#include <pvol.h>
#include <dtypes.h>
#include <Application.h>

#include "Renderer.h"
#include "Camera.h"
#include "Datasets.h"
#include "Visualization.h"
#include "OSPRayRenderer.h"

#include "Rendering.h"
#include "RenderingSet.h"

static int width = 1920, height = 1080;
static int mpiRank, mpiSize;

char  *
timestamp()
{
  time_t timer;
  time(&timer);
  return asctime(localtime(&timer));
}

void
syntax(string executable)
{
	if (GetTheApplication()->GetRank() == 0)
	{
		std::cerr << "syntax: " << executable << " state.json [options]\n";
		std::cerr << "options:\n";
		std::cerr << "  -s w h            width and height of image (" << width << ", " << height << ")\n";
  	std::cerr << "  -D dbg            debug - dbg is -1 for all, or k to debug rank k\n";
    std::cerr << "  -f fname          filename (defaults to xyzzy)\n";
    std::cerr << "  -C                cinema filenames (./cinema.cdb/image/img + annotations)\n";
    std::cerr << "  -S                synchronous - each V/C Rendering in a separate RenderingSet, rendered 1 by 1\n";
		GetTheApplication()->QuitApplication();
	}
	exit(0);
}

int
main(int argc, char * argv[])
{
	string statefile("");
	char *dbg = NULL;
	int iKnt = 1;
	int oKnt = 1;
	bool write_image = true;
  string fname = "state";
  bool synchronous = false;

	Application theApplication(&argc, &argv);

	theApplication.Start();

	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-s"))      { width = atoi(argv[++i]); height = atoi(argv[++i]); }
		else if (!strcmp(argv[i], "-I")) iKnt = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-O")) oKnt = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-D")) dbg = argv[++i];
		else if (!strcmp(argv[i], "-W")) write_image = false;
		else if (!strcmp(argv[i], "-f")) fname = argv[++i];
		else if (!strcmp(argv[i], "-C")) fname = "./cinema.cdb/image/img";
		else if (!strcmp(argv[i], "-S")) synchronous = true;
		else if (statefile == "")        statefile = argv[i];
		else syntax(argv[0]);
	}

	setup_debugger(argv[0]);
	debugger(dbg);

  RaycastRenderer  *theRenderer = (RaycastRenderer *) new OSPRayRenderer(&argc, &argv);

	theApplication.Run();
	mpiRank = theApplication.GetRank();
	mpiSize = theApplication.GetSize();

	if (mpiRank == 0)
	{

		Document *doc = GetTheApplication()->OpenInputState(statefile);

		DatasetsP theDatasets = Datasets::NewP();
		theDatasets->LoadFromJSON(*doc);

		std::cerr << "Application start: " << timestamp();

		vector<CameraP> theCameras = Camera::LoadCamerasFromJSON(*doc);
		for (auto c : theCameras)
			c->Commit();

		vector<VisualizationP> theVisualizations = Visualization::LoadVisualizationsFromJSON(*doc);

		theRenderer->LoadStateFromDocument(doc);
		theRenderer->Commit();

		delete doc;


		auto t1 = std::chrono::high_resolution_clock::now();

		theDatasets->Commit();
		
		for (auto v : theVisualizations)
				v->Commit(theDatasets);
				
		theRenderer->Commit();

		for (auto c : theCameras)
				c->Commit();

    vector<RenderingSetP> theRenderingSets;

    if (synchronous)
    {

      for (auto c : theCameras)
          for (auto v : theVisualizations)
          {
              RenderingSetP rs = RenderingSet::NewP();
              theRenderingSets.push_back(rs);

              RenderingP theRendering = Rendering::NewP();
              theRendering->SetTheOwner(0);
              theRendering->SetTheSize(width, height);
              theRendering->SetTheCamera(c);
              theRendering->SetTheDatasets(theDatasets);
              theRendering->SetTheVisualization(v);
              theRendering->SetTheRenderingSet(rs);
              theRendering->Commit();
              rs->AddRendering(theRendering);
              rs->Commit();
          }


    }
    else
    {
      RenderingSetP rs = RenderingSet::NewP();
      theRenderingSets.push_back(rs);

      int index = 0;
      for (auto c : theCameras)
          for (auto v : theVisualizations)
          {
              RenderingP theRendering = Rendering::NewP();
              theRendering->SetTheOwner(index++ % mpiSize );
              theRendering->SetTheSize(width, height);
              theRendering->SetTheCamera(c);
              theRendering->SetTheDatasets(theDatasets);
              theRendering->SetTheVisualization(v);
              theRendering->SetTheRenderingSet(rs);
              theRendering->Commit();
              rs->AddRendering(theRendering);
          }

      rs->Commit();

    }

    int indx = 0;
    for (auto rs : theRenderingSets)
    {
      theRenderer->Render(rs);
      rs->WaitForDone();
      std::cout << "rs " << indx++ << " done\n";
    }

		auto t2 = std::chrono::high_resolution_clock::now();
		std::cerr << "rendering time " << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() << " milliseconds\n";

		sleep(5);

    for (auto rs : theRenderingSets)
      rs->SaveImages(fname);

		theApplication.QuitApplication();
		std::cerr << "Application finish: " << timestamp();
	}

	theApplication.Wait();

	delete theRenderer;
}
