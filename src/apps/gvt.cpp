// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
// All rights reserved.                                                       //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// A copy of the License is included with this software in the file LICENSE.  //
// If your copy does not contain the License, you may obtain a copy of the    //
// License at:                                                                //
//                                                                            //
//     https://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  //
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
//                                                                            //
// ========================================================================== //

#include <iostream>
#include <vector>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

// #include <ospray/ospray.h>

#include "galaxy.h"

#include <Application.h>
#include <Renderer.h>
#include <Camera.h>
//#include <Volume.h>
#include <AmrVolume.h>
#include <Datasets.h>
#include <VolumeVis.h>
#include <Visualization.h>

using namespace gxy;

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
        std::cerr << "Attach to PID " << pid << std::endl;
      else
      { 
        cmd << "~/dbg_script " << executable << " " << pid << " &";
        system(cmd.str().c_str());
      }
      
      while (dbg)
        sleep(1);
      
      std::cerr << "running" << std::endl;
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
	std::cerr << "rank: " << mpiRank << std::endl;

	Debug *d = (argc > 1 && !strcmp(argv[1], "-D")) ?  new Debug(argv[0], false, argv[1]) : NULL;

	theApplication.Run();

  RendererP theRenderer = Renderer::NewP();

	if (theApplication.GetRank() == 0)
	{
		theRenderer->Commit();

		CameraP c = Camera::NewP();
		c->set_viewpoint(3.0, 3.0,3.0);
		c->set_viewdirection(-4.0, -4.0, -4.0);

		//c->set_viewpoint(0.0, 0.0, 4.0);
		//c->set_viewdirection(0.0, 0.0, -2.0);

		c->set_viewup(0.0, 0.0, 1.0);
		c->set_angle_of_view(30.0);
		c->Commit();

		// VolumeP v = Volume::NewP();
		// v->Import("oneBall-0.vol");
        
		AmrVolumeP v = AmrVolume::NewP();
		v->Import("ballinthecorner.amrvol");
		v->Commit();

		DatasetsP d = Datasets::NewP();
	        d->Insert("oneBall", v);
		d->Commit();

		VolumeVisP vv = VolumeVis::NewP();
		vv->SetName("oneBall");
		//vec4f slice(0.0,0.0, 1.0, 0.25);
		//vv->AddSlice(slice);
		vv->AddIsovalue(5.0);
		vv->AddIsovalue(1.0);
        //vv->SetVolumeRendering(true);
    
    vec4f cmap[] = {
        {0.00,1.0,0.5,0.5},
        {17.25,0.5,1.0,0.5},
        {35.50,0.5,0.5,1.0},
        {52.75,1.0,1.0,0.5},
        {72.00,1.0,0.5,1.0}
    };
    /*vec4f cmap[] = {
        {0.00,1.0,0.5,0.5},
        {0.25,0.5,1.0,0.5},
        {0.50,0.5,0.5,1.0},
        {0.75,1.0,1.0,0.5},
        {1.00,1.0,0.5,1.0}
    };*/
    
    vv->SetColorMap(5, cmap);

    vec2f omap[] = {
                { 0.00, 1.00 },
                { 0.20, 1.00 },
                { 0.71, 1.00 },
                { 71.00, 1.00 }
               };

    vv->SetOpacityMap(4, omap);

		vv->Commit(d);


		VisualizationP vis = Visualization::NewP();
		vis->AddVis(vv);
    Lighting *l = vis->get_the_lights();
    float camera_relative[] = {5.0, 5.0, 0.0};
    int   type[] = { 1 };
    l->SetLights(1, camera_relative, type);
		vis->Commit(d);

		RenderingP r = Rendering::NewP();
		r->SetTheOwner(0);
        c->set_width(512);
        c->set_height(512);
		r->SetTheCamera(c);
		r->SetTheDatasets(d);
		r->SetTheVisualization(vis);
		r->Commit();

		RenderingSetP rs = RenderingSet::NewP();
		rs->AddRendering(r);
		rs->Commit();

		theRenderer->Start(rs);

#ifdef GXY_WRITE_IMAGES
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
