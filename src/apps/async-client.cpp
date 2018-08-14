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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <pthread.h>

#include "Socket.h"
#include "async.h"
#include "Pixel.h"
#include "ImageWriter.h"

using namespace gxy;
using namespace std;

int mpiRank = 0, mpiSize = 1;

#include "Debug.h"

#define WIDTH  500
#define HEIGHT 500
int width = WIDTH, height = HEIGHT;

ImageWriter image_writer("async_client");

static pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;

float*       pixels = NULL;
float*       negative_pixels = NULL;
int*         frameids = NULL;
int*         negative_frameids = NULL;
long*        frame_times = NULL;
volatile int frame = -1;
pthread_t 	 receiver_tid;
int 				 max_f = -1;
int					 fknt[1000];
bool 				 no_mouse = false;

Socket *skt;

long
my_time()
{
  timespec s;
  clock_gettime(CLOCK_REALTIME, &s);
  return 1000000000*s.tv_sec + s.tv_nsec;
}

int rcvd = 0;

void 
clear()
{
	rcvd = 0;
	for (int i = 0; i < width*height*4; i++)
		pixels[i] = ((i&0x3) == 0x3) ? 1.0 : 0.0;
}

void
SendResetCamera()
{
	glClear(GL_COLOR_BUFFER_BIT);
	clear();
  int op = RESET_CAMERA;
	skt->Send((char *)&op, sizeof(op));
}

void
SendStats()
{
  int op = STATS;
	skt->Send((char *)&op, sizeof(op));
}

void
SendSync()
{
  int op = SYNCHRONIZE;
	skt->Send((char *)&op, sizeof(op));
}

void
SendDebug()
{
  int op = DEBUG;
	skt->Send((char *)&op, sizeof(op));
}
	
void
SendRenderOne()
{
  int op = RENDER_ONE;
	skt->Send((char *)&op, sizeof(op));
}
	
void
SendStart(cam_mode mode, int w, int h, string statefile)
{
	int sz = sizeof(struct start) + statefile.length() + 1;
	struct start *buf = (struct start *)new char[sz];

	buf->op = START;
	buf->mode = mode;
	buf->width = w;
	buf->height = h;
	memcpy(&buf->name[0], statefile.c_str(), statefile.length()+1);

	skt->Send((char *)buf, sz);
	delete[] buf;
}

void
SendMouseDown(int k, int s, float x, float y)
{
	struct mouse_down md;
	md.op = MOUSEDOWN;
	md.k = k;
	md.s = s;
	md.x = x;
	md.y = y;
	skt->Send((char *)&md, sizeof(md));
}

void
SendMouseMotion(float x, float y)
{
	struct mouse_motion mm;
	mm.op = MOUSEMOTION;
	mm.x = x;
	mm.y = y;
	skt->Send((char *)&mm, sizeof(mm));
}

void
SendQuit()
{
	int op = QUIT;
	skt->Send((char *)&op, sizeof(op));

	pthread_join(receiver_tid, NULL);
}

void
draw(void)
{
	if (pixels)
	{
		glDrawPixels(width, height, GL_RGBA, GL_FLOAT, pixels);
		glutSwapBuffers();
	}
}

void
keyboard(unsigned char ch, int x, int y)
{
  switch (ch)
	{
		case 0x63: // c reset camera
			SendResetCamera();
			break;
		case 0x73: // s Sync
			SendSync();
			break;
		case 0x54: // T stats
			SendStats();
			break;
		case 0x51: // Q - print rcvd
			std::cerr << rcvd << std::endl;
			break;
		case 0x53: // S - save image
			image_writer.Write(width, height, pixels);
			break;
		case 0x52: // R - render a single 
			clear();
			SendRenderOne();
			break;
		case 0x44:
			for (int i = 0; i <= max_f; i++)
				std::cerr << i << ": " << fknt[i] << std::endl;
			SendDebug();
			break;
    case 0x1B: 
			SendQuit();
      exit(0);
		case 0x40:
			glutPostRedisplay();
			break;
  }
}

void
menu(int item)
{
  switch (item) {
		case 0x54: // T stats
			SendStats();
			break;
		case 0x52:
			SendRenderOne();
			break;
    case 0x1B: 
			SendQuit();
      exit(0);
  }
}

void
mousefunc(int k, int s, int x, int y)
{
	if (no_mouse) return;
  if (s == GLUT_DOWN)
		SendMouseDown(k, s, -1.0 + 2.0*(float(x)/width), -1.0 + 2.0*(float(y)/height));
}
  
void
motionfunc(int x, int y)
{
	if (no_mouse) return;
	SendMouseMotion(-1.0 + 2.0*(float(x)/width), -1.0 + 2.0*(float(y)/height));
}

void *
ager_thread(void *)
{
	while (true)
	{
		struct timespec rm, tm = {0, 100000000};
    nanosleep(&tm, &rm);

		pthread_mutex_lock(&buffer_lock);

		long now = my_time();

		for (int offset = 0; offset < width*height; offset++)
		{
			int fid = frameids[offset];
			if (fid > 0 && fid < max_f)
			{
				long tm = frame_times[offset];
        float sec = (now - tm) / 1000000000.0;

        float *pix = pixels + (offset*4);
        if (sec > max_f)
        {
          frame_times[offset] = now;
          frameids[offset] = max_f;
          *pix++ = 0.0, *pix++ = 0.0, *pix++ = 0.0, *pix++ = 1.0;
        }
        else
          *pix++ *= 0.9, *pix++ *= 0.9, *pix++ *= 0.9, *pix++ = 1.0;
      }
		}

		pthread_mutex_unlock(&buffer_lock);
	}
}
	
void *
receiver_thread(void *)
{
	char *buf; int n;
	static int frame = 0;
  
	while (true)
	{
		skt->Recv(buf, n);

		char *ptr = buf;
		int knt = *(int *)ptr;
		ptr += sizeof(int);
		int frame = *(int *)ptr;
		ptr += sizeof(int);
		int sender = *(int *)ptr;
		ptr += sizeof(int);
		Pixel *p = (Pixel *)ptr;

// std::cerr << frame << " " << knt << "\n";

		long now = my_time();
		if (frame >= max_f)
		{
			rcvd += knt;

 	 		for (int i = max_f + 1; i <= frame;  i++)
				fknt[i] = 0;

			fknt[frame] += knt;

			pthread_mutex_lock(&buffer_lock);

			for (int i = 0; i < knt; i++, p++)
			{
				size_t offset = p->y*width + p->x;
				float *pix = pixels + (offset<<2);
				float *npix = negative_pixels + (offset<<2);

				// If its a sample from the current frame, add it in.
				// 
				// If its a sample from a LATER frame then two possibilities:
				// its a negative sample, in which case we DON'T want to update
				// the visible pixel, so we stash it. If its a POSITIVE sample
				// we stuff the visible pixel with the sample and any stashed
				// negative samples.
				//
				if (frameids[offset] == frame)
				{
					*pix++ += p->r;
					*pix++ += p->g;
					*pix++ += p->b;
				}
				else
				{
					if (p->r < 0.0 || p->g < 0.0 || p->b < 0.0)
					{
						// If its a NEGATIVE sample and...
						
						if (negative_frameids[offset] == frame)
						{
							// from the current frame, we add it to the stash
							*npix++ += p->r;
							*npix++ += p->g;
							*npix++ += p->b;
						}
						else if (negative_frameids[offset] < frame)
						{
							// otherwise its from a LATER frame, so we init the stash so if
							// we receive any POSITIVE samples we can add it in.
							negative_frameids[offset] = frame;
							*npix++ = p->r;
							*npix++ = p->g;
							*npix++ = p->b;
						}
					}
					else
					{
						// its a POSITIVE sample from a NEW frame, so we stuff the visible
						// pixel with the new sample and any negative samples that arrived 
						// first
						max_f = frame;
						frameids[offset] = frame;
						frame_times[offset] = now;
						if (max_f == negative_frameids[offset])
						{
							*pix++ = (*npix + p->r), *npix = 0.0, npix++;
							*pix++ = (*npix + p->g), *npix = 0.0, npix++;
							*pix++ = (*npix + p->b), *npix = 0.0, npix++;
						}
						else
						{
							*pix++ = p->r;
							*pix++ = p->g;
							*pix++ = p->b;
						}
					}
				}
			}

			pthread_mutex_unlock(&buffer_lock);
		}

		free(buf);
	}

	pthread_exit(NULL);
}

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options] statefile" << endl;
  cerr << "options:" << endl;
  cerr << "  -D         run debugger" << endl;
	cerr << "  -H host    host (localhost)" << endl;
	cerr << "  -P port		port (5001)" << endl;
  cerr << "  -A         wait for attachment" << endl;
  cerr << "  -s w h     image size (512 x 512)" << endl;
  cerr << "  -F         ignore mouse movement" << endl;
  exit(1);
}

int
main(int argc, char *argv[])
{
  bool dbg = false, atch = false;
	char *dbgarg;
	string host = "localhost";
	string statefile = "";
	int port = 5001;
	cam_mode mode = OBJECT_CENTER;

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-A")) dbg = true, atch = true;
    else if (!strcmp(argv[i], "-D")) dbg = true, atch = false, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i], "-H")) host = argv[++i];
    else if (!strcmp(argv[i], "-P")) port = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-F")) no_mouse = true;
    else if (!strcmp(argv[i], "-O")) mode = OBJECT_CENTER;
    else if (!strcmp(argv[i], "-E")) mode = EYE_CENTER;
    else if (!strcmp(argv[i], "-s"))
    {
      width  = atoi(argv[++i]);
      height = atoi(argv[++i]);
    }
    else if (statefile == "")
			statefile = argv[i];
		else
      syntax(argv[0]);
  }

	if (statefile == "")
		syntax(argv[0]);

  Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;
	skt = new Socket((char *)host.c_str(), port);

	negative_pixels    = (float *)malloc(width*height*4*sizeof(float));
	pixels      		   = (float *)malloc(width*height*4*sizeof(float));
	negative_frameids  = (int *)malloc(width*height*sizeof(int));
	frameids    		   = (int *)malloc(width*height*sizeof(int));
	frame_times 		   = (long *)malloc(width*height*sizeof(long));

	long now = my_time();
	memset(frameids, 0, width*height*sizeof(int));
	memset(negative_frameids, 0, width*height*sizeof(int));
	for (int i = 0; i < width*height; i++)
		frame_times[i] = now;

	for (int i = 0; i < 4*width*height; i++)
	{
		pixels[i] = (i & 0x3) == 0x3 ? 1.0 : 0.0;
		negative_pixels[i] = (i & 0x3) == 0x3 ? 1.0 : 0.0;
	}

	pthread_t receiver_tid;
	pthread_create(&receiver_tid, NULL, receiver_thread, NULL);

	pthread_t ager_tid;
	pthread_create(&ager_tid, NULL, ager_thread, NULL);

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(width, height);
  glutCreateWindow("render");
  glutIdleFunc(draw);
  glutDisplayFunc(draw);
  glutMotionFunc(motionfunc);
  glutMouseFunc(mousefunc);
  glutKeyboardFunc(keyboard);
  glutCreateMenu(menu);
  glutAddMenuEntry("Quit", QUIT);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glClearDepth(1.0);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glMatrixMode(GL_PROJECTION);
  glOrtho(-1, 1, -1, 1, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glRasterPos2i(-1, -1);
  frame = 1;

	SendStart(mode, width, height, statefile);

  glutMainLoop();

  return 0;
}
