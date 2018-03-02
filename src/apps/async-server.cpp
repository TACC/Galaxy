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
vec4f current_rotation;

float orig_viewdistance, orig_aov;
vec3f orig_viewpoint, orig_viewdirection, orig_viewup;
vec4f orig_current_rotation;

vec3f down_viewdirection, down_viewpoint;
float down_viewdistance;

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
  orig_viewdistance = len(orig_viewdirection);

  normalize(orig_viewdirection);
  normalize(orig_viewup);

  vec3f viewright;
  cross(orig_viewdirection, orig_viewup, viewright);
  cross(viewright, orig_viewdirection, orig_viewup);

  vec3f y(0.0, 1.0, 0.0);
  float ay = acos(dot(y, viewup));
  axis_to_quat(orig_viewdirection, ay, orig_current_rotation);

  viewdistance = orig_viewdistance;
  current_rotation = orig_current_rotation;
  viewpoint = orig_viewpoint;
  viewdirection = orig_viewdirection;
  viewup = orig_viewup;
  aov = orig_aov;

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
		float x1 = X1, y1 = Y1;   // So stays unclanged by other thread during rendering

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
            vec4f this_rotation;
            trackball(this_rotation, X0, Y0, X1, Y1);

            vec4f next_rotation;
            add_quats(this_rotation, current_rotation, next_rotation);
            current_rotation = next_rotation;

            vec3f y(0.0, 1.0, 0.0);
            vec3f z(0.0, 0.0, 1.0);

            vec3f center = viewpoint + viewdirection*viewdistance;
            rotate_vector_by_quat(y, current_rotation, viewup);
            rotate_vector_by_quat(z, current_rotation, viewdirection);
            viewpoint = center - viewdirection*viewdistance;
          }
          else
          {
            vec3f center = down_viewpoint + down_viewdirection*down_viewdistance;
            float d = (Y1 > Yd) ? 2.0 * ((Y1 - Yd) / (2.0 - Yd)) : -2.0 * ((Y1 - Yd)/(-2.0 - Yd));
            viewdistance = down_viewdistance * pow(10.0, d);
            viewpoint = center - viewdirection*viewdistance;
          }
        }
        else
        {
          if (button == 0)
          {
            vec4f this_rotation;
            trackball(this_rotation, X0, Y0, X1, Y1);

            vec4f next_rotation;
            add_quats(this_rotation, current_rotation, next_rotation);
            current_rotation = next_rotation;

            vec3f y(0.0, 1.0, 0.0);
            vec3f z(0.0, 0.0, 1.0);

            rotate_vector_by_quat(y, current_rotation, viewup);
            rotate_vector_by_quat(z, current_rotation, viewdirection);
          }
          else
          {
            float d = (Y1 > Yd) ? 2.0 * ((Y1 - Yd) / (2.0 - Yd)) : -2.0 * ((Y1 - Yd)/(-2.0 - Yd));
            std::cerr << d<<"\n";
            viewpoint = down_viewpoint + viewdirection*d;
          }
        }
			}

			X0 = X1; Y0 = Y1;

			theCamera->set_viewdirection(viewdirection); 
			theCamera->set_viewpoint(viewpoint);
			theCamera->set_viewup(viewup);
			theCamera->set_angle_of_view(aov);
			theCamera->Commit();

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

			switch(op)
			{
				case RESET_CAMERA:
					viewdistance = orig_viewdistance;
					current_rotation = orig_current_rotation;
					viewpoint = orig_viewpoint;
					viewdirection = orig_viewdirection;
					viewup = orig_viewup;
					aov = orig_aov;
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

						down_viewdistance = viewdistance;
						down_viewpoint = viewpoint;
						down_viewdirection = viewdirection;

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
