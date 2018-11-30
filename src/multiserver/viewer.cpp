// ========================================================================== //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 //
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
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <pthread.h>
#include <time.h>

#include "dtypes.h"
#include "Pixel.h"
#include "ImageWriter.h"
#include "ClientWindow.h"
#include "IF.h"

#include "trackball.hpp"

#include <ospray/ospray.h>

using namespace gxy;
using namespace std;

enum cam_mode
{
  OBJECT_CENTER, EYE_CENTER
} mode;

#define WIDTH  500
#define HEIGHT 500

int width = WIDTH, height = HEIGHT;
float Xd, Yd;
float X0, Y0;
float X1, Y1;
int button = -1;

bool window_ready = false;

float age = 3.0;
float fadeout = 1.0;

int  *pargc;
char **pargv;

string statefile("");

ImageWriter image_writer("async");

bool  render_one = false;

float orig_viewdistance, orig_aov;
vec3f orig_viewpoint, orig_viewdirection, orig_viewup;
vec3f center;
vec3f	down_viewpoint;
float down_scaling;

Trackball trackball;
float scaling;

ClientWindow *theClientWindow;
CameraIF theCamera;

int mpiRank = 0, mpiSize = 1;
#include "Debug.h"

void
quit()
{
  cerr << "quit\n";
  exit(0);
}

void
Render()
{
  if (window_ready)
  {
    string s("render");
    if (! theClientWindow->GetSocket()->CSendRecv(s))
    {
      cerr << "render request send failed\n";
      exit(1);
    }
  }
}

void
LoadState(string sfile)
{
  ifstream t(sfile);
  if (t.fail())
  {
    cerr << "state file " << sfile << "not found\n";
    return;
  }

  stringstream buffer;
  buffer << "json " << t.rdbuf();

  string s = buffer.str();
  theCamera.LoadString((const char *)(s.c_str() + 4));

  if (! theClientWindow->GetSocket()->CSendRecv(s))
  {
    cerr << "JSON send failed\n";
    exit(1);
  }

  // cerr << "reply to sending JSON: " << s << "\n";

  Render();
}

float *xyzzy = NULL;

void
draw(void)
{
  float *pixels = theClientWindow->GetPixels();
	if (pixels)
	{
    if (xyzzy == NULL)
      xyzzy = pixels;
    else if (xyzzy != pixels)
      std::cerr << "PIXELS ERROR!\n";

		float *p = pixels + 3;
		for (int i = 0; i < (theClientWindow->GetWidth()*theClientWindow->GetHeight()); i++)
			*p = 1.0, p += 4;
		
		glDrawPixels(theClientWindow->GetWidth(), theClientWindow->GetHeight(), GL_RGBA, GL_FLOAT, pixels);
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
    case 0x51:  // Q
      quit();
      break;

    case 0x44: // D
        {
          string s("query datasets");

          if (! theClientWindow->GetSocket()->CSendRecv(s))
          {
            cerr << "sending query datasets failed\n";
            exit(1);
          }
          cerr << "Datasets: " << s << "\n";
        }
        break; 

		case 0x52: // R
			render_one = true;
			break;


    case 0x62: // b reset theCamera
		  {
			 	trackball.spin(0.0, 0.0, 0.05, 0.0);
				vec3f d = trackball.rotate_vector(orig_viewdirection);
				vec3f u = trackball.rotate_vector(orig_viewup);
				vec3f p = center - d * orig_viewdistance * scaling;
				theCamera.set_viewpoint(p);
				theCamera.set_viewup(u);
				theCamera.set_viewdirection(d);
				theCamera.Send(theClientWindow->GetSocket());
				render_one = true;
			}
			break;


    case 0x43: // C show theCamera
            {
                float p[3];
                theCamera.get_viewpoint(p);
                cerr << "P: " << p[0] << " " << p[1] << " " << p[2] << "\n";
                theCamera.get_viewdirection(p);
                cerr << "D: " << p[0] << " " << p[1] << " " << p[2] << "\n";
                theCamera.get_viewup(p);
                cerr << "U: " << p[0] << " " << p[1] << " " << p[2] << "\n";
                float a;
                theCamera.get_angle_of_view(a);
                cerr << "U: " << a << "\n";
            }
            break;

 
    case 0x63: // c reset theCamera
			{
				trackball.reset();
				scaling = 1;
				X1 = X0;
				Y1 = Y0;
				trackball.reset();
				vec3f d = trackball.rotate_vector(orig_viewdirection);
				vec3f u = trackball.rotate_vector(orig_viewup);
				vec3f p = center - d * orig_viewdistance * scaling;
				theCamera.set_viewpoint(p);
				theCamera.set_viewup(u);
				theCamera.set_viewdirection(d);
				theCamera.Send(theClientWindow->GetSocket());
				render_one = true;
			}
			break;

    case 0x54: // T
      theClientWindow->partial_frame_test();
      render_one = true;
      break;

		case 0x53:
		case 0x73:
				image_writer.Write(theClientWindow->GetWidth(), theClientWindow->GetHeight(), theClientWindow->GetPixels());
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
				image_writer.Write(theClientWindow->GetWidth(), theClientWindow->GetHeight(), theClientWindow->GetPixels());
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
reshapefunc(int w, int h)
{
  cerr << "reshape: " << w << " " << h << "\n";
  stringstream buffer;
  buffer << "window " << w << " " << h;

  string s = buffer.str();
  if (! theClientWindow->GetSocket()->CSendRecv(s))
  {
    cerr << "sending reshape failed\n";
    exit(0);
  }

  // cerr << "reshape reply: " << s << "\n";
}

void
glut_loop()
{
  glutInit(pargc, pargv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(width, height);
  glutCreateWindow("render");
  glutReshapeFunc(reshapefunc);
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

  window_ready = true;

  glutMainLoop();
}

void *
render_thread(void *d)
{
  theCamera.get_viewpoint(orig_viewpoint);
  theCamera.get_viewdirection(orig_viewdirection);
  theCamera.get_viewup(orig_viewup);
  theCamera.get_angle_of_view(orig_aov);

	center = orig_viewpoint + orig_viewdirection;

	orig_viewdistance = len(orig_viewdirection);
  normalize(orig_viewdirection);

  scaling = 1.0;

  vec3f viewright;
  cross(orig_viewdirection, orig_viewup, viewright);
  cross(viewright, orig_viewdirection, orig_viewup);
	normalize(viewright);
	orig_viewup = cross(viewright, orig_viewdirection);
	theCamera.set_viewup(orig_viewup);

	bool first = true;

	while (true)
	{
		if (render_one || first || (X0 != X1) || (Y0 != Y1))
		{
			first = false;
			if ((X0 != X1) || (Y0 != Y1))
			{
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

					theCamera.set_viewdirection(d);
					theCamera.set_viewpoint(p);
					theCamera.set_viewup(u);
          theCamera.Send(theClientWindow->GetSocket());
				}
				else
				{
					if (button == 0)
					{
						trackball.spin(X0, Y0, X1, Y1);
						vec3f d = trackball.rotate_vector(orig_viewdirection);
						theCamera.set_viewdirection(d);
            theCamera.Send(theClientWindow->GetSocket());
					}
				}
			}

      Render();

      // cerr << "reply to sending render request: " << s << "\n";
      render_one = false;

			X0 = X1; Y0 = Y1;
		}

		struct timespec t;
		t.tv_sec  = 0;
		t.tv_nsec = 100000000;
		nanosleep(&t, NULL);
	}

	return 0;
}

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options] statefile" << endl;
  cerr << "options:" << endl;
  cerr << "  -H host          host (localhost)" << endl;
  cerr << "  -P port          port (5001)" << endl;
  cerr << "  -D[which]        run debugger in selected processes.  If which is given, it is a number or a hyphenated range, defaults to all" << endl;
  cerr << "  -so sofile       interface SO (libviewer-if.so)\n";
  cerr << "  -A               wait for attachment" << endl;
  cerr << "  -s w h           image size (512 x 512)" << endl;
  cerr << "  -O               object-center model (default)" << endl;
  cerr << "  -E               eye-center model" << endl;
	cerr << "  -a age fadeout   sample age to begin fadeout (3.0), fadeout (1.0)" << endl;
  cerr << "  -D               run debugger" << endl;
  exit(1);
}

int
main(int argc, char *argv[])
{
  bool dbg = false, atch = false;
  string host = "localhost";
  string statefile = "";
  string sofile = "libviewerlib.so";
  int port = 5001;

	char *dbgarg;

  pargc = &argc;
  pargv = argv;

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-a")) { age = atof(argv[++i]), fadeout = atof(argv[++i]); }
    else if (!strcmp(argv[i], "-H")) host = argv[++i];
    else if (!strncmp(argv[i],"-D", 2)) dbg = true, atch = false, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i], "-P")) port = atoi(argv[++i]);
		else if (!strncmp(argv[i], "-D", 2)) { dbg = true, dbgarg = argv[i] + 2; break; }
    else if (!strcmp(argv[i], "-O")) { mode = OBJECT_CENTER; }
    else if (!strcmp(argv[i], "-E")) { mode = EYE_CENTER; }
    else if (!strcmp(argv[i], "-so")) { sofile = argv[++i]; }
    else if (!strcmp(argv[i], "-s"))
    {
      width  = atoi(argv[++i]);
      height = atoi(argv[++i]);
    }
    else if (statefile == "")
    { statefile = argv[i]; }
    else
    { syntax(argv[0]); }
  }

  if (statefile == "")
    syntax(argv[0]);

  Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;

  theClientWindow = new ClientWindow(width, height);
  theClientWindow->Connect(host.c_str(), port);
  
  string rply;

  if (! theClientWindow->GetSocket()->CSendRecv(sofile))
  {
    cerr << "Sending sofile failed\n";
    exit(1);
  }
  // cerr << "reply from sending sofile: " << sofile << "\n";

  LoadState(statefile);

  pthread_t tid;
  pthread_create(&tid, NULL, render_thread, (void *)theClientWindow);

  glut_loop();

  return 0;
}
