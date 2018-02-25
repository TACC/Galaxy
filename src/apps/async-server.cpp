#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "Socket.h"
#include "Application.h"
#include "Renderer.h"
#include "ServerRendering.h"
#include <ospray/ospray.h>

#include "Debug.h"

using namespace std;

#include "async.h"
#include "quat.h"

Socket *skt;

bool render_one = false;

float X0, Y0;
float X1, Y1;
float *buf = NULL;
bool quit = false;

ServerRenderingP 	theRendering = NULL;
RenderingSetP   	theRenderingSet = NULL;
CameraP         	theCamera = NULL;
VisualizationP  	theVisualization = NULL;

RendererP theRenderer;

void *
render_thread(void *buf)
{
	int width = ((int *)buf)[1];
	int height = ((int *)buf)[2];
	string statefile = string(((char *)buf) + 3*sizeof(int));

  theRenderer = Renderer::NewP();
  Document *doc = GetTheApplication()->OpenInputState(statefile);
  theRenderer->LoadStateFromDocument(*doc);

	free(buf);

  vector<CameraP> theCameras = Camera::LoadCamerasFromJSON(*doc);
  theCamera = theCameras[0];

  vec3f scaled_viewdirection, center, viewpoint, viewdirection, viewup;
  theCamera->get_viewpoint(viewpoint);
  theCamera->get_viewdirection(viewdirection);
  theCamera->get_viewup(viewup);
  add(viewpoint, viewdirection, center);
	
  float aov, viewdistance = len(viewdirection);
	theCamera->get_angle_of_view(aov);

  normalize(viewdirection);
  normalize(viewup);

	scaled_viewdirection = viewdirection;
	scale(viewdistance, scaled_viewdirection);

  vec3f viewright;
  cross(viewdirection, viewup, viewright);
  cross(viewright, viewdirection, viewup);

  vec3f y(0.0, 1.0, 0.0);
  float ay = acos(dot(y, viewup));

  vec4f current_rotation;
  axis_to_quat(viewdirection, ay, current_rotation);

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

#if 0
	int frame = 0;
	cerr << "---------------- " << frame++ << "\n";
	cerr << "dir:  " << viewdirection.x << " " << viewdirection.y << " " << viewdirection.z << "\n";
	cerr << "sdir: " << scaled_viewdirection.x << " " << scaled_viewdirection.y << " " << scaled_viewdirection.z << "\n";
	cerr << "vp:   " << viewpoint.x << " " << viewpoint.y << " " << viewpoint.z << "\n";
	cerr << "up:   " << viewup.x << " " << viewup.y << " " << viewup.z << "\n";
	cerr << "aov:  " << aov << "\n";
	cerr << "vdist:" << viewdistance << "\n";
#endif

	theRenderer->Render(theRenderingSet);

	while (! quit)
	{
		float x1 = X1, y1 = Y1;   // So stays unclanged by other thread during rendering

		float dx = (x1 - X0);
		float dy = (y1 - Y0);

		bool cam_moved = sqrt(dx*dx + dy*dy) > 0.001;
		if (render_one || cam_moved)
		{
			render_one = false;

			if (cam_moved)
			{
				vec4f this_rotation;
				trackball(this_rotation, X0, Y0, x1, y1);

				vec4f next_rotation;
				add_quats(this_rotation, current_rotation, next_rotation);
				current_rotation = next_rotation;

				vec3f y(0.0, 1.0, 0.0);
				vec3f z(0.0, 0.0, 1.0);

				rotate_vector_by_quat(y, current_rotation, viewup);
				rotate_vector_by_quat(z, current_rotation, viewdirection);

				scaled_viewdirection = viewdirection;
				scale(viewdistance, scaled_viewdirection);

				sub(center, scaled_viewdirection, viewpoint);
				X0 = x1; Y0 = y1;
			}

#if 0
      if (theRenderingSet)
      {
        theRenderingSet->Drop();
        theRenderingSet = NULL;
      }

      if (theRendering)
      {
        theRendering->Drop();
        theRendering = NULL;
      }

      if (theCamera)
      {
        theCamera->Drop();
        theCamera = NULL;
      }

			cerr << "---------------- " << frame++ << "\n";
			cerr << "dir:  " << viewdirection.x << " " << viewdirection.y << " " << viewdirection.z << "\n";
			cerr << "sdir: " << scaled_viewdirection.x << " " << scaled_viewdirection.y << " " << scaled_viewdirection.z << "\n";
			cerr << "vp:   " << viewpoint.x << " " << viewpoint.y << " " << viewpoint.z << "\n";
			cerr << "up:   " << viewup.x << " " << viewup.y << " " << viewup.z << "\n";
			cerr << "aov:  " << aov << "\n";
			cerr << "vdist:" << viewdistance << "\n";

			theCamera = Camera::NewP();
			theCamera->set_viewdirection(scaled_viewdirection); 
			theCamera->set_viewpoint(viewpoint);
			theCamera->set_viewup(viewup);
			theCamera->set_angle_of_view(aov);
			theCamera->Commit();

			theRendering = ServerRendering::NewP();
			theRendering->SetSocket(skt);
			theRendering->SetTheOwner(0);
			theRendering->SetTheSize(width, height);
			theRendering->SetTheDatasets(theDatasets);
			theRendering->SetTheVisualization(theVisualization);
			theRendering->SetTheCamera(theCamera);
			theRendering->Commit();

			theRenderingSet = RenderingSet::NewP();
			theRenderingSet->AddRendering(theRendering);
			theRenderingSet->Commit();
#else
			theCamera->set_viewdirection(scaled_viewdirection); 
			theCamera->set_viewpoint(viewpoint);
			theCamera->set_viewup(viewup);
			theCamera->set_angle_of_view(aov);
			theCamera->Commit();
#endif
			theRenderer->Render(theRenderingSet);
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
			void *args = (void *)(buf + sizeof(int));

			switch(*(int *)buf)
			{
				case SYNC:
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
					X0 = X1 = ((float *)args)[0];
					Y0 = Y1 = ((float *)args)[1];
					free(buf);
					break;

				case MOUSEMOTION:
					X1 = ((float *)args)[0];
					Y1 = ((float *)args)[1];
					free(buf);
					break;
			}
		}
	}
  else
    GetTheApplication()->Wait();

  return 0;
}
