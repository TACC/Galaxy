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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <pthread.h>

#include "Socket.h"
#include "async.h"

#include "Pixel.h"
#include "ImageWriter.h"

using namespace gxy;
using namespace std;

int mpiRank, mpiSize;

#include "Debug.h"

#define WIDTH  500
#define HEIGHT 500
int width = WIDTH, height = HEIGHT;

float*       pixels = NULL;
int*         frameids = NULL;
volatile int frame = -1;
pthread_t 	 receiver_tid;

Socket *skt;

ImageWriter image_writer("async-client-no-x");
void save_image();

void SendDebug()
{
  int n = DEBUG;
	skt->Send((char *)&n, sizeof(n));
}

void
SendRenderOne()
{
  int n = RENDER_ONE;
	skt->Send((char *)&n, sizeof(n));
}
	
void
SendStart(int w, int h, string statefile)
{
	int sz = 3*sizeof(int) + statefile.length() + 1;
	char *buf = new char[sz];

	char *p = buf;
	*(int *)p = START;
	p += sizeof(int);
	*(int *)p = w;
	p += sizeof(int);
	*(int *)p = h;
	p += sizeof(int);
	memcpy(p, statefile.c_str(), statefile.length()+1);

	skt->Send(buf, sz);
	delete[] buf;
}

void
SendMouseDown(float x, float y)
{
	cerr << "md " << x << " " << y << endl;
	int sz = sizeof(int) + 2*sizeof(float);
	char *buf = new char[sz];

	char *p = buf;
	*(int *)p = MOUSEDOWN;
	p += sizeof(int);
	*(float *)p = x;
	p += sizeof(float);
	*(float *)p = y;

	skt->Send(buf, sz);
	delete[] buf;
}

void
SendMouseMotion(float x, float y)
{
	cerr << "mm " << x << " " << y << endl;
	int sz = sizeof(int) + 2*sizeof(float);
	char *buf = new char[sz];

	char *p = buf;
	*(int *)p = MOUSEMOTION;
	p += sizeof(int);
	*(float *)p = x;
	p += sizeof(float);
	*(float *)p = y;

	skt->Send(buf, sz);
	delete[] buf;
}

void
SendQuit()
{
	int sz = sizeof(int);
	char *buf = new char[sz];

	char *p = buf;
	*(int *)p = QUIT;

	skt->Send(buf, sz);
	delete[] buf;

	pthread_join(receiver_tid, NULL);
}

int max_f = -1;
int fknt[1000];

void *
receiver_thread(void *)
{
	char *buf; int n;
  
	while (true)
	{
		skt->Recv(buf, n);

		char *ptr = buf;
		int knt = *(int *)ptr;
		ptr += sizeof(int);
		int frame = *(int *)ptr;
		ptr += sizeof(int);
		Pixel *p = (Pixel *)ptr;

		if (frame > max_f)
		{
 	 		for (int i = max_f + 1; i <= frame;  i++)
				fknt[i] = 0;
			max_f = frame;
			fknt[frame] += knt;

			for (int i = 0; i < knt; i++, p++)
			{
				// size_t offset = (((height-1)-(p->y))*width + ((width-1)-(p->x)));
				size_t offset = (p->y*width + p->x);

				float *pix = pixels + (offset<<2);
				if (frameids[offset] < frame)
				{
					pix[0] = 0;
					pix[1] = 0;
					pix[2] = 0;
					pix[3] = 0;
				}
				*pix++ += p->r;
				*pix++ += p->g;
				*pix++ += p->b;
				*pix++ += p->o;
			}
		}
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

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-A")) dbg = true, atch = true;
    else if (!strcmp(argv[i], "-D")) dbg = true, atch = false, dbgarg = argv[i] + 2;
    else if (!strcmp(argv[i], "-H")) host = argv[++i];
    else if (!strcmp(argv[i], "-P")) port = atoi(argv[++i]);
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

	pthread_t receiver_tid;
	pthread_create(&receiver_tid, NULL, receiver_thread, NULL);

	pixels = new float[width*height*4];
	frameids = new int[width*height*4];
	float *p = pixels; int *f = frameids;
	for (int i = 0; i < width*height; i++)
		*f++ = 0, *p++ = 0.0, *p++ = 0.0, *p++ = 0.0, *p++ = 0.0;

	SendStart(width, height, statefile);

  string cmd;
  float x, y;

  do
  {
    cin >> cmd;
    if (cmd == "d")
    {
			float x, y;
      cin >> x >> y;
      SendMouseDown(x, y);
		}
    else if (cmd == "m")
    {
			float x, y;
      cin >> x >> y;
      SendMouseMotion(x, y);
		}
    else if (cmd == "D")
			SendDebug();
    else if (cmd == "r")
			SendRenderOne();
    else if (cmd == "S")
			save_image();
  }
  while (cmd != "q");

	delete[] pixels;
	delete[] frameids;

  return 0;
}

void
save_image()
{
	image_writer.Write(width, height, pixels);
}
