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
#include <string.h>
#include <unistd.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <pthread.h>
#include <time.h>

#include "ImageWriter.h"
#include "Application.h"
#include "Renderer.h"
#include "AsyncRendering.h"
#include "MHSampleMsg.h"
#include "trackball.hpp"
#include "gxyviewer.h"

#include <ospray/ospray.h>

using namespace gxy;
using namespace std;
using namespace rapidjson;

int mpiRank;
int mpiSize;

#include "Debug.h"

#define WIDTH  500
#define HEIGHT 500

int width = WIDTH, height = HEIGHT;
float Xd, Yd;
float X0, Y0;
float X1, Y1;
int button = -1;

float age = 3.0;
float fadeout = 1.0;

float radius = 0.02;
float sigma = 1.0;
int   max_consecutive_misses = 10;
int   iteration_limit = 200000;
int   hot_start = 1000;
float sigma_scaling = 1.0;
int   skip = 10;
float data_min = -1, data_max = -1;
bool  data_minmax_from_argument = false;

volatile int frame = -1;

int  *pargc;
char **pargv;

string statefile("");
string volfile("");

ImageWriter image_writer("gyxviewer");

AsyncRenderingP	theRendering = NULL;
RenderingSetP 	theRenderingSet = NULL;
CameraP 				theCamera = NULL;
VisualizationP 	theVisualization = NULL;
DatasetsP 			theDatasets = NULL;
ParticlesP			theParticles = NULL;
VolumeP					theVolume = NULL;
ParticlesVisP   theParticlesVis = NULL;
VolumeVisP    	theVolumeVis = NULL;

float *pixels = NULL;
bool  render_one = false;

float orig_viewdistance, orig_aov;
vec3f orig_viewpoint, orig_viewdirection, orig_viewup;
vec3f center;

vec3f	down_viewpoint;
float down_scaling;

Trackball trackball;
float scaling;

cam_mode mode = OBJECT_CENTER;

void
Sample()
{
	MHSampleMsg *smsg = new MHSampleMsg(theVolume, theParticles, data_min, data_max, sigma_scaling, iteration_limit, hot_start, skip, max_consecutive_misses);
	smsg->Broadcast(true, true);
	theParticles->Commit();
}

void SetupVolume()
{
	theVolume = Volume::NewP();
	theVolume->Import(volfile);

	if (! data_minmax_from_argument)
		theVolume->get_global_minmax(data_min, data_max);
}

void SetupParticles()
{
	theParticles = Particles::NewP();
	theParticles->SetRadius(radius);
}

void 
SetupVisualization()
{
	theParticlesVis = ParticlesVis::NewP();
	theParticlesVis->SetName("samples");
	theParticlesVis->Commit(theDatasets);

	theVolumeVis = VolumeVis::NewP();
	theVolumeVis->SetName("volume");

	vec4f cmap[] = {
        {0.00,1.0,0.5,0.5},
        {0.25,0.5,1.0,0.5},
        {0.50,0.5,0.5,1.0},
        {0.75,1.0,1.0,0.5},
        {1.00,1.0,0.5,1.0}
	};

	vec2f omap[] = {
                { 0.00, 0.00},
                { 1.00, 0.10},
                { 8.00, 0.10}
	};

	theVolumeVis->SetColorMap(5, cmap);
	theVolumeVis->SetOpacityMap(3, omap);
	theVolumeVis->SetVolumeRendering(true);
	theVolumeVis->Commit(theDatasets);

	theVisualization = Visualization::NewP();
	theVisualization->AddVis(theParticlesVis);
	theVisualization->AddVis(theVolumeVis);
	theVisualization->Commit(theDatasets);

	theRendering->SetTheVisualization(theVisualization);
	theRendering->Commit();
	theRenderingSet->Commit();

	pixels = theRendering->GetPixels();
}

enum _state
{
	RUNNING,
	QUITTING,
	DONE
} state;

pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  state_wait = PTHREAD_COND_INITIALIZER;

_state
get_state()
{
  pthread_mutex_lock(&state_lock);
  _state s = state;
	pthread_mutex_unlock(&state_lock);
	return s;
}

void
set_state(_state s)
{
  pthread_mutex_lock(&state_lock);
  state  = s;
	pthread_cond_signal(&state_wait);
	pthread_mutex_unlock(&state_lock);
}

void
wait_state(_state s)
{
	pthread_mutex_lock(&state_lock);
	while (s != state)
		pthread_cond_wait(&state_wait, &state_lock);
	pthread_mutex_unlock(&state_lock);
}

void
draw(void)
{
	if (pixels)
	{
		float *p = pixels + 3;
		for (int i = 0; i < (width*height); i++)
			*p = 1.0, p += 4;
		
		glDrawPixels(width, height, GL_RGBA, GL_FLOAT, pixels);
		glutSwapBuffers();
	}
}

void
animate(void)
{
  glutPostRedisplay();
}

static int inum = 0;
	
void
keyboard(unsigned char ch, int x, int y)
{
  switch (ch) {
		case 0x52:
			render_one = true;
			break;


    case 0x62: // b reset camera
		  {
			 	trackball.spin(0.0, 0.0, 0.05, 0.0);
				vec3f d = trackball.rotate_vector(orig_viewdirection);
				vec3f u = trackball.rotate_vector(orig_viewup);
				vec3f p = center - d * orig_viewdistance * scaling;
				theCamera->set_viewpoint(p);
				theCamera->set_viewup(u);
				theCamera->set_viewdirection(d);
				theCamera->Commit();
				render_one = true;
			}
			break;


    case 0x63: // c reset camera
			{
				trackball.reset();
				scaling = 1;
				X1 = X0;
				Y1 = Y0;
				trackball.reset();
				vec3f d = trackball.rotate_vector(orig_viewdirection);
				vec3f u = trackball.rotate_vector(orig_viewup);
				vec3f p = center - d * orig_viewdistance * scaling;
				theCamera->set_viewpoint(p);
				theCamera->set_viewup(u);
				theCamera->set_viewdirection(d);
				theCamera->Commit();
				render_one = true;
			}
			break;

    case 0x54: // T
      theRendering->partial_frame_test();
      render_one = true;
      break;

    case 0x53: // S
			Sample();
			SetupVisualization();
      render_one = true;
      break;

		case 0x73:
				image_writer.Write(width, height, pixels);
				break;

    case 0x1B: 
			set_state(QUITTING);
			wait_state(DONE);
      GetTheApplication()->QuitApplication();
      GetTheApplication()->Wait();
      exit(0);
      break;

		case 0x40:
			glutPostRedisplay();
			break;
  }
}

// #define QUIT    1

void
menu(int item)
{
  switch (item) {
		case 0x52:
			render_one = true;
			break;

		case 0x53:
		case 0x73:
				image_writer.Write(width, height, pixels);
				break;

    case 0x1B: 
			set_state(QUITTING);
			wait_state(DONE);

      GetTheApplication()->QuitApplication();
      GetTheApplication()->Wait();

      exit(0);
      break;
  }
}

void
mousefunc(int k, int s, int x, int y)
{
  if (s == GLUT_DOWN)
  {
		button = k;
    Xd = X1 = X0 = -1.0 + 2.0*(float(x)/WIDTH);
    Yd = Y1 = Y0 = -1.0 + 2.0*(float(y)/HEIGHT);
		down_scaling = scaling;
  }
	else
	{
		button = -1;
	}
}
  
void
motionfunc(int x, int y)
{
	X1 = -1.0 + 2.0*(float(x)/WIDTH);
	Y1 = -1.0 + 2.0*(float(y)/HEIGHT);
}

void
glut_loop()
{
  glutInit(pargc, pargv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(width, height);
  glutCreateWindow("render");
  glutIdleFunc(draw);
  glutDisplayFunc(draw);
  glutMotionFunc(motionfunc);
  glutMouseFunc(mousefunc);
  glutKeyboardFunc(keyboard);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glClearDepth(1.0);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glMatrixMode(GL_PROJECTION);
  glOrtho(-1, 1, -1, 1, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glRasterPos2i(-1, -1);
  frame = 1;
  glutMainLoop();
}

void *
render_thread(void *d)
{
  RendererP theRenderer = Renderer::NewP();

  Document *doc = GetTheApplication()->OpenJSONFile(statefile);
  theRenderer->LoadStateFromDocument(*doc);

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
	theCamera->Commit();

	SetupVolume();
	SetupParticles();

	theDatasets = Datasets::NewP();
	theDatasets->Insert("samples", theParticles);
	theDatasets->Insert("volume", theVolume);
	theDatasets->Commit();

	theRendering = AsyncRendering::NewP();
	theRendering->SetMaxAge(age, fadeout);
	theRendering->SetTheOwner(0);
	theRendering->SetTheSize(width, height);
	theRendering->SetTheDatasets(theDatasets);
	theRendering->SetTheCamera(theCamera);

	theRenderingSet = RenderingSet::NewP();
	theRenderingSet->AddRendering(theRendering);

	Sample();

	SetupVisualization();


	bool first = true;

	while (get_state() == RUNNING)
	{
		if (render_one || first || (X0 != X1) || (Y0 != Y1))
		{
			first = false;
			if ((X0 != X1) || (Y0 != Y1))
			{
				// Y0 = Y1 = 0.0;
				if (mode == OBJECT_CENTER)
				{
					if (button == 0)
					{
						trackball.spin(X0, Y0, X1, Y1);
					}
					else if (button != -1)
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
						viewpoint = down_viewpoint + viewdirection*d;
					}
#endif
				}
			}

			if (render_one)
				render_one = false;

			theRenderer->Render(theRenderingSet);
			X0 = X1; Y0 = Y1;
		}

		struct timespec t;
		t.tv_sec  = 0;
		t.tv_nsec = 100000000;
		nanosleep(&t, NULL);
	}

	set_state(DONE);
	return 0;
}

void
syntax(char *a)
{
  cerr << "asynchronous viewer for Metropolis-Hastings sampling" << endl;
  cerr << "syntax: " << a << endl;
  cerr << "options:" << endl;
	cerr << "  -J json          state file to get renderer and camera info (required)" << endl;
	cerr << "  -V vol           volfile (required)" << endl;

  cerr << "  -M m          max number of consecutive misses allowed (" << max_consecutive_misses << ")" << "\n";
  cerr << "  -i n          iteration limit (" << iteration_limit << ")\n";
  cerr << "  -h n          hot start count (" << hot_start << ")\n";
  cerr << "  -S step       scaling of candidate step (" << sigma_scaling << ")\n";
  cerr << "  -K skip       record every skip'th successful sample (" << skip << ")\n";
  cerr << "  -R p0 p1      values at which probability is 0 and 1, extended (data range min to max)\n";
  cerr << "  -s x y        window size (" << WIDTH << "x" << HEIGHT << ")" << endl;
  cerr << "  -r radius     radius of samples (" << radius << ")" << endl;
  cerr << "  -k sigma      standard deviation of normally distributed random step vector (" << sigma << ")" << endl;

  cerr << "  -D               run debugger" << endl;
  cerr << "  -A               wait for attachment" << endl;
  cerr << "  -O               object-center model (default)" << endl;
  cerr << "  -E               eye-center model" << endl;
	cerr << "  -a age fadeout   sample age to begin fadeout (3.0), fadeout (1.0)" << endl;
  exit(1);
}

int
main(int argc, char *argv[])
{
  bool dbg = false, atch = false;
	char *dbgarg;

  pargc = &argc;
  pargv = argv;

  ospInit(pargc, (const char **)argv);

  Application theApplication(&argc, &argv);
  theApplication.Start();

  AsyncRendering::RegisterClass();

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-a")) { age = atof(argv[++i]), fadeout = atof(argv[++i]); }
		else if (!strcmp(argv[i], "-D")) { dbg = true, dbgarg = argv[i] + 2; break; }
    else if (!strcmp(argv[i], "-O")) { mode = OBJECT_CENTER; }
    else if (!strcmp(argv[i], "-J")) { statefile = argv[++i]; }
    else if (!strcmp(argv[i], "-V")) { volfile = argv[++i]; }
    else if (!strcmp(argv[i], "-E")) { mode = EYE_CENTER; }
    else if (!strcmp(argv[i], "-M")) { max_consecutive_misses = atoi(argv[++i]); }
    else if (!strcmp(argv[i], "-i")) { iteration_limit = atoi(argv[++i]); }
    else if (!strcmp(argv[i], "-h")) { hot_start = atoi(argv[++i]); }
    else if (!strcmp(argv[i], "-S")) { scaling = atof(argv[++i]); }
    else if (!strcmp(argv[i], "-K")) { skip = atoi(argv[++i]); }
    else if (!strcmp(argv[i], "-R")) 
		{
			data_min = atof(argv[++i]);
		  data_max = atof(argv[++i]);
		  data_minmax_from_argument = true;
		}
    else if (!strcmp(argv[i], "-r")) { radius = atof(argv[++i]); }
    else if (!strcmp(argv[i], "-k")) { sigma = atof(argv[++i]); }
    else if (!strcmp(argv[i], "-s"))
    {
      width  = atoi(argv[++i]);
      height = atoi(argv[++i]);
    }
    else
    { syntax(argv[0]); }
  }

  if (statefile == "" || volfile == "")
    syntax(argv[0]);


  Renderer::Initialize();
  GetTheApplication()->Run();

  mpiRank = GetTheApplication()->GetRank();
  mpiSize = GetTheApplication()->GetSize();

  Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;

	MHSampleMsg::Register();

  if (mpiRank == 0)
  {
    pthread_t tid;
    pthread_create(&tid, NULL, render_thread, NULL);

    glut_loop();
  }
  else
    GetTheApplication()->Wait();

  return 0;
}
