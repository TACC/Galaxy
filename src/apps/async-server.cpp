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

#include "Debug.h"

using namespace std;

#include "async.h"
#include "trackball.hpp"

using namespace pvol;

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
  Document *doc = GetTheApplication()->OpenInputState(statefile);
  theRenderer->LoadStateFromDocument(*doc);

	free(buf);

  vector<CameraP> theCameras = Camera::LoadCamerasFromJSON(*doc);
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
}

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options]\n";
  cerr << "options:\n";
  cerr << "  -D         run debugger\n";
  cerr << "  -A         wait for attachment\n";
  cerr << "  -P port    port to use (5001)\n";
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

  Debug *d = dbg ? new Debug(argv[0], atch) : NULL;

  Renderer::Initialize();
  GetTheApplication()->Run();

  if (GetTheApplication()->GetRank() == 0)
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
						std::cerr << "rendering thread already running!\n";
						exit(0);
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
