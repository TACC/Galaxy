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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include <ospray/ospray.h>

#include "Socket.h"
#include "Application.h"
#include "Renderer.h"
#include "ServerRendering.h"

int mpiRank, mpiSize;

#include "Debug.h"

#include "async.h"
#include "trackball.hpp"

using namespace gxy;
using namespace std;
using namespace rapidjson;

Socket *skt;

bool render_one = false;

float Xd, X0, Y0;
float Yd, X1, Y1;
int button;
int button_state;
cam_mode mode;

float *buf = NULL;
bool quit = false;

ServerRenderingP 	theRendering = NULL;
RenderingSetP   	theRenderingSet = NULL;
CameraP         	theCamera = NULL;
VisualizationP  	theVisualization = NULL;

RendererP theRenderer;

float viewdistance, aov;
vec3f viewpoint, viewdirection, viewup;

float orig_viewdistance, orig_aov;
vec3f orig_viewpoint, orig_viewdirection, orig_viewup;
vec3f center;

vec3f down_viewpoint;
float down_scaling;

Trackball trackball;
float scaling;

void *
render_thread(void *buf)
{
	struct start *start = (struct start *)buf;

	int mode = start->mode;
	int width = start->width;
	int height = start->height;
	string statefile = string(start->name);

  theRenderer = Renderer::NewP();
  Document *doc = GetTheApplication()->OpenJSONFile(statefile);
  theRenderer->LoadStateFromDocument(*doc);

	free(buf);

  vector<CameraP> theCameras;
  Camera::LoadCamerasFromJSON(*doc, theCameras);
  theCamera = theCameras[0];

  theCamera->get_viewpoint(orig_viewpoint);
  theCamera->get_viewdirection(orig_viewdirection);
  theCamera->get_viewup(orig_viewup);
  theCamera->get_angle_of_view(orig_aov);

  center = orig_viewpoint + orig_viewdirection;

  orig_viewdistance = len(orig_viewdirection);
  normalize(orig_viewdirection);

	scaling = 1.0;

  vec3f viewright;
  cross(orig_viewdirection, orig_viewup, viewright);
  cross(viewright, orig_viewdirection, orig_viewup);
  normalize(viewright);
  orig_viewup = cross(viewright, orig_viewdirection);
  theCamera->set_viewup(orig_viewup);

  DatasetsP theDatasets = Datasets::NewP();
  theDatasets->LoadFromJSON(*doc);
  theDatasets->Commit();

  vector<VisualizationP> theVisualizations = Visualization::LoadVisualizationsFromJSON(*doc);
  theVisualization = theVisualizations[0];
	theVisualization->Commit(theDatasets);

  theRendering = ServerRendering::NewP();

	theRendering->SetSocket(skt);
  theRendering->SetTheOwner(0);
  theRendering->SetTheSize(width, height);
  theRendering->SetTheDatasets(theDatasets);
  theRendering->SetTheVisualization(theVisualization);
	theRendering->SetTheCamera(theCamera);
  theRendering->Commit();

  RenderingSetP theRenderingSet = RenderingSet::NewP();
  theRenderingSet->AddRendering(theRendering);
  theRenderingSet->Commit();

	theRenderer->Render(theRenderingSet);

	while (! quit)
	{
		float x1 = X1, y1 = Y1;

		float dx = (x1 - X0);
		float dy = (y1 - Y0);

		bool cam_moved = sqrt(dx*dx + dy*dy) > 0.001;
		if (render_one || cam_moved)
		{
			render_one = false;

			if (cam_moved)
			{
        if (mode == OBJECT_CENTER)
        {
          if (button == 0)
          {
            trackball.spin(X0, Y1, X1, Y0);
          }
          else
          {
            float d = (Y1 > Yd) ? 2.0 * ((Y1 - Yd) / (2.0 - Yd)) : -2.0 * ((Y1 - Yd)/(-2.0 - Yd));
            scaling = down_scaling * pow(10.0, d);
          }

          vec3f d = trackball.rotate_vector(orig_viewdirection);
          vec3f u = trackball.rotate_vector(orig_viewup);
          vec3f p = center - d * orig_viewdistance * scaling;
  
          theCamera->set_viewdirection(d);
          theCamera->set_viewpoint(p);
          theCamera->set_viewup(u);
          theCamera->Commit(); 

        }
        else
        {
          if (button == 0)
          {
						trackball.spin(X0, Y0, X1, Y1);
            vec3f d = trackball.rotate_vector(orig_viewdirection);
            theCamera->set_viewdirection(d);
            theCamera->Commit();
          }
#if 0
          else
          {
            float d = (Y1 > Yd) ? 2.0 * ((Y1 - Yd) / (2.0 - Yd)) : -2.0 * ((Y1 - Yd)/(-2.0 - Yd));
            std::cerr << d<<"\n";
            viewpoint = down_viewpoint + viewdirection*d;
          }
#endif
        }
			}

			X0 = X1; Y0 = Y1;

			theRenderer->Render(theRenderingSet);

			timespec ts = {0, 100000000};
			nanosleep(&ts, NULL);
		}
	}
	return 0;
}

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options]" << endl;
  cerr << "options:" << endl;
  cerr << "  -D         run debugger" << endl;
  cerr << "  -A         wait for attachment" << endl;
  cerr << "  -P port    port to use (5001)" << endl;
  exit(1);
}

int
main(int argc, char *argv[])
{
  bool dbg = false, atch = false;
	int port = 5001;

  ospInit(&argc, (const char **)argv);

  Application theApplication(&argc, &argv);
  theApplication.Start();

	ServerRendering::RegisterClass();

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-A")) dbg = true, atch = true;
    else if (!strcmp(argv[i], "-D")) dbg = true, atch = false;
    else if (!strcmp(argv[i], "-P")) port = atoi(argv[++i]);
    else
      syntax(argv[0]);
  }

	mpiRank = GetTheApplication()->GetRank();
	mpiSize = GetTheApplication()->GetSize();

  Debug *d = dbg ? new Debug(argv[0], atch, "") : NULL;

  Renderer::Initialize();
  GetTheApplication()->Run();

  if (mpiRank == 0)
	{
		skt = new Socket(port);

		pthread_t render_tid = 0;

		while (!quit)
		{
			char *buf; int n;
			skt->Recv(buf, n);

			int op = *(int *)buf;

			switch(op)
			{
				case RESET_CAMERA:
					viewdistance = orig_viewdistance;
					viewpoint = orig_viewpoint;
					viewdirection = orig_viewdirection;
					viewup = orig_viewup;
					aov = orig_aov;
					trackball.reset();
					render_one = true;
					break;

				case SYNCHRONIZE:
					theApplication.SyncApplication();
					break;

				case STATS: 
					theRenderer->DumpStatistics();
					break;

				case RENDER_ONE:
					render_one = true;
					break;

				case QUIT:
					quit = true;
					free(buf);
					pthread_join(render_tid, NULL);
					break;
		
				case START:
					if (render_tid != 0)
					{
						std::cerr << "START op received, but rendering thread already running!" << endl;
						exit(1);
					}
					pthread_create(&render_tid, NULL, render_thread, buf); // buf, so lower level can delete
					break;
				
				case MOUSEDOWN:
					{
						struct mouse_down *md = (struct mouse_down *)buf;

						button_state = md->s;
						button = md->k;
						Xd = X0 = X1 = md->x;
						Yd = Y0 = Y1 = md->y;

						down_viewpoint = viewpoint;
						down_scaling = scaling; 
						free(buf);
					}
					break;

				case MOUSEMOTION:
					{
						struct mouse_motion *mm = (struct mouse_motion *)buf;
						X1 = mm->x;
						Y1 = mm->y;
						free(buf);
					}
					break;
			}
		}
	}
  else
    GetTheApplication()->Wait();

  return 0;
}
