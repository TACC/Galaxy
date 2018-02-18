#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <pthread.h>

#include "Debug.h"
#include "Socket.h"
#include "async.h"
#include <png.h>

#include "Pixel.h"

#include "mypng.h"

using namespace std;

#define WIDTH  500
#define HEIGHT 500
int width = WIDTH, height = HEIGHT;

float*       pixels = NULL;
int*         frameids = NULL;
volatile int frame = -1;
pthread_t 	 receiver_tid;

Socket *skt;

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
	std::cerr << "md " << x << " " << y << "\n";
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
	std::cerr << "mm " << x << " " << y << "\n";
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
				size_t offset = (((height-1)-(p->y))*width + ((width-1)-(p->x)));
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
  cerr << "syntax: " << a << " [options] statefile\n";
  cerr << "options:\n";
  cerr << "  -D         run debugger\n";
	cerr << "  -H host    host (localhost)\n";
	cerr << "  -P port		port (5001)\n";
  cerr << "  -A         wait for attachment\n";
  cerr << "  -s w h     image size (512 x 512)\n";
  exit(1);
}

int
main(int argc, char *argv[])
{
  bool dbg = false, atch = false;
	string host = "localhost";
	string statefile = "";
	int port = 5001;

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-A")) dbg = true, atch = true;
    else if (!strcmp(argv[i], "-D")) dbg = true, atch = false;
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

  Debug *d = dbg ? new Debug(argv[0], atch) : NULL;
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

void png_error(png_structp png_ptr, png_const_charp error_msg)
{
  cerr << "PNG error: " << error_msg << "\n";
}

void png_warning(png_structp png_ptr, png_const_charp warning_msg)
{
  cerr << "PNG error: " << warning_msg << "\n";
}

int write_png(const char *filename, int w, int h, unsigned int *rgba)
{
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL, png_warning, png_error);
  if (!png_ptr)
  {
    cerr << "Unable to create PNG write structure\n";
    return 0;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    cerr << "Unable to create PNG info structure\n";
    return 0;
  }

  FILE *fp = fopen(filename, "wb");
  if (! fp)
  {
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    cerr << "Unable to open PNG file: " << filename << "\n";
    return 0;
  }

  png_init_io(png_ptr, fp);

  png_set_IHDR(png_ptr, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGB_ALPHA,
  	PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  png_byte **rows = new png_byte *[h];

  for (int i = 0; i < h; i++)
    rows[i] = (png_bytep)(rgba + i*w);

  png_set_rows(png_ptr, info_ptr, rows);
  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  png_destroy_write_struct(&png_ptr, &info_ptr);
  free(rows);

  fclose(fp);

  return 1;
}

void
save_image()
{
  static int frame = 0;
	char name[256];
	sprintf(name, "frame-%04d.png", frame++);
	unsigned char *buf = new unsigned char[4*width*height];
	float *p = pixels; unsigned char *b = buf;
	for (int i = 0; i < width*height; i++)
  {
    *b++ = (unsigned char)(255*(*p++));
    *b++ = (unsigned char)(255*(*p++));
    *b++ = (unsigned char)(255*(*p++));
    *b++ = 0xff ; p ++;
  }
	write_png(name, width, height, (unsigned int *)buf);
	delete[] buf;
}
