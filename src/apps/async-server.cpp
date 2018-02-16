#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "Application.h"
#include "Renderer.h"
#include <ospray/ospray.h>

#include "Debug.h"

using namespace std;

#include "async.h"
#include "quat.h"
#include "async-server.h"
#include "Socket.h"

Socket *skt;

KEYED_OBJECT_TYPE(ServerRendering)

void
ServerRendering::initialize()
{
  Rendering::initialize();
}

void
ServerRendering::AddLocalPixels(Pixel *p, int n, int f)
{
	char* ptrs[] = {(char *)&n, (char *)&f, (char *)p};
	int   szs[] = {sizeof(int), sizeof(int), static_cast<int>(n*sizeof(Pixel)), 0};

	skt->SendV(ptrs, szs);

	Rendering::AddLocalPixels(p, n, f);
}

float X0, Y0;
float X1, Y1;
volatile int frame = -1;
float *buf = NULL;
bool quit = false;

void *
render_thread(void *buf)
{
	int width = ((int *)buf)[1];
	int height = ((int *)buf)[2];
	string statefile = string(((char *)buf) + 3*sizeof(int));

  RendererP theRenderer = Renderer::NewP();
  Document *doc = GetTheApplication()->OpenInputState(statefile);
  theRenderer->LoadStateFromDocument(*doc);

	free(buf);

  vector<CameraP> theCameras = Camera::LoadCamerasFromJSON(*doc);
  CameraP theCamera = theCameras[0];

  vec3f viewpoint, viewdirection, viewup;
  theCamera->get_viewpoint(viewpoint);
  theCamera->get_viewdirection(viewdirection);
  theCamera->get_viewup(viewup);

  vec3f center;
  add(viewpoint, viewdirection, center);

  float viewdistance = len(viewdirection);

  normalize(viewdirection);
  normalize(viewup);

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
  VisualizationP v = theVisualizations[0];
	v->Commit(theDatasets);

  RenderingSetP theRenderingSet = RenderingSet::NewP();

  RenderingP theRendering = ServerRendering::NewP();
  theRendering->SetTheOwner(0);
  theRendering->SetTheSize(width, height);
  theRendering->SetTheDatasets(theDatasets);
  theRendering->SetTheVisualization(v);
  theRendering->SetTheRenderingSet(theRenderingSet);
	theRendering->SetTheCamera(theCamera);
  theRendering->Commit();

  theRenderingSet->AddRendering(theRendering);
  theRenderingSet->Commit();

	buf = theRendering->GetPixels();

	theCamera->Commit();
	theRenderer->Render(theRenderingSet);

	while (! quit)
	{
		if ((X0 != X1) || (Y0 != Y1))
		{
			vec4f this_rotation;
			trackball(this_rotation, X0, Y0, X1, Y1);

			vec4f next_rotation;
			add_quats(this_rotation, current_rotation, next_rotation);
			current_rotation = next_rotation;

			vec3f y(0.0, 1.0, 0.0);
			vec3f z(0.0, 0.0, 1.0);

			rotate_vector_by_quat(y, current_rotation, viewup);
			theCamera->set_viewup(viewup);

			rotate_vector_by_quat(z, current_rotation, viewdirection);

			vec3f dir = viewdirection;
			scale(viewdistance, dir);
			theCamera->set_viewdirection(dir);

			sub(center, dir, viewpoint);
			theCamera->set_viewpoint(viewpoint);

			theCamera->Commit();
			theRenderer->Render(theRenderingSet);

			X0 = X1; Y0 = Y1;
		}
	}
}

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options] statefile\n";
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
	skt = new Socket(port);

  Renderer::Initialize();
  GetTheApplication()->Run();

  if (GetTheApplication()->GetRank() == 0)
	{
		pthread_t render_tid = 0;

		while (!quit)
		{
			char *buf; int n;
			skt->Recv(buf, n);

			int op = *(int *)buf;
			void *args = (void *)(buf + sizeof(int));

			switch(*(int *)buf)
			{
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
					std::cerr << "md " << X0 << " " << Y0 << "\n";
					free(buf);
					break;

				case MOUSEMOTION:
					X1 = ((float *)args)[0];
					Y1 = ((float *)args)[1];
					std::cerr << "mm " << X1 << " " << Y1 << "\n";
					free(buf);
					break;
			}
		}
	}
  else
    GetTheApplication()->Wait();

  return 0;
}
