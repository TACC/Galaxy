#include <string>
#include <iostream>
#include <sstream>

#include "Application.h"
#include "Renderer.h"

#include <ospray/ospray.h>

void
syntax(char *a)
{
  std::cerr << "syntax: " << a << " [options] json\n";
  std::cerr << "optons:\n";
  std::cerr << "  -D         run debugger\n";
  std::cerr << "  -A         wait for attachment\n";
  std::cerr << "  -s w h     window width, height (256 256)\n";
	exit(1);
}

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

int main(int argc,  char *argv[])
{
	string statefile("");
	bool dbg = false;
	bool atch = false;
	int width = 256, height = 256;

	ospInit(&argc, (const char **)argv);

	Application theApplication(&argc, &argv);
  theApplication.Start();


  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-A")) dbg = true, atch = true;
    else if (!strcmp(argv[i], "-D")) dbg = true, atch = false;
    else if (!strcmp(argv[i], "-s")) width = atoi(argv[++i]), height = atoi(argv[++i]);
    else if (statefile == "")   statefile = argv[i];
    else syntax(argv[0]);
  }

	if (statefile == "")
		syntax(argv[0]);

	Debug *d = dbg ? new Debug(argv[0], atch) : NULL;

	Renderer::Initialize();

	theApplication.Run();

  int mpiRank = theApplication.GetRank();
  int mpiSize = theApplication.GetSize();

	if (mpiRank == 0)
	{
		RendererP theRenderer = Renderer::NewP();

    Document *doc = GetTheApplication()->OpenInputState(statefile);

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

		for (auto rs : theRenderingSets)
    {
      theRenderer->Render(rs);
      rs->WaitForDone();
			rs->SaveImages("image");
    }

    theApplication.QuitApplication();
  }

  theApplication.Wait();
}
