#include <iostream>
#include <vector>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <ospray/ospray.h>

#include "pvol.h"

#include <Application.h>
#include <Renderer.h>
#include <Camera.h>
#include <Volume.h>
#include <Datasets.h>
#include <VolumeVis.h>
#include <Visualization.h>

using namespace pvol;

int mpiRank;

class Debug
{
public:
  Debug(const char *executable, bool attach, char *arg)
  { 
    bool dbg = true;
    std::stringstream cmd;
    pid_t pid = getpid();
    
    bool do_me = true;
    if (arg && *arg)
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

int
main(int argc, char * argv[])
{
  ospInit(&argc, (const char **)argv);

	Application theApplication(&argc, &argv);
	theApplication.Start(false);

	Renderer::Initialize();

	mpiRank = theApplication.GetRank();
	std::cerr << "rank: " << mpiRank << "\n";

	Debug *d = (argc > 1 && !strcmp(argv[1], "-D")) ?  new Debug(argv[0], false, argv[1]) : NULL;

	theApplication.Run();

	if (theApplication.GetRank() == 0)
	{
		RendererP theRenderer = Renderer::NewP();
		theRenderer->Commit();
		std::cerr << "renderer initted\n";

		CameraP c = Camera::NewP();
		c->set_viewpoint(0.0, 0.0, -5.0);
		c->set_viewdirection(0.0, 0.0, 1.0);
		c->set_viewup(0.0, 1.0, 0.0);
		c->set_angle_of_view(45.0);
		c->Commit();

		VolumeP v = Volume::NewP();
		v->Import("oneBall-0.vol");
		v->Commit();

		DatasetsP d = Datasets::NewP();
	  d->Insert("oneBall", v);
		d->Commit();

		VolumeVisP vv = VolumeVis::NewP();
		vv->SetName("oneBall");
		vec4f slice(0.0, 0.0, -1.0, 0.0);
		vv->AddSlice(slice);
		vv->AddIsovalue(0.25);
		vv->Commit(d);

		VisualizationP vis = Visualization::NewP();
		vis->AddVolumeVis(vv);
		vis->Commit(d);

		RenderingP r = Rendering::NewP();
		r->SetTheOwner(0);
		r->SetTheSize(200, 200);
		r->SetTheCamera(c);
		r->SetTheDatasets(d);
		r->SetTheVisualization(vis);
		r->Commit();

		RenderingSetP rs = RenderingSet::NewP();
		rs->AddRendering(r);
		rs->Commit();

		theRenderer->Render(rs);

#ifdef PVOL_SYNCHRONOUS
		rs->WaitForDone();
#else
		std::cerr << "hit a char when you are ready to write the image: ";
		getchar();
#endif

		rs->SaveImages("gvt");

		theApplication.QuitApplication();
	}

	theApplication.Wait();
}
