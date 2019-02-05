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

#include "ClientWindow.h"
#include "ImageWriter.h"
#include <string>
#include <sstream>
#include <pthread.h>
#include <time.h>

namespace gxy
{

ClientWindow::ClientWindow(int width, int height, std::string host, int port) : SocketHandler(host, port)
{
  current_frame     = -1;
	max_age           = 3.0;
	fadeout           = 1.0;

  pixels            = NULL;
  negative_pixels   = NULL;
  frameids          = NULL;
  negative_frameids = NULL;
  frame_times       = NULL;

  kill_threads      = false;

	t_start = my_time();

	pthread_mutex_init(&lock, NULL);

  save_partial_updates     = false;
  number_of_partial_frames = -1;
  this_frame_pixel_count   = 0;

  std::string so("libgxy_module_viewer.so");
  if (! CSendRecv(so) || so != "ok")
  {
    std::cerr << "Server-side library load failed: " << so << "\n";
    exit(1);
  }

  Resize(width, height);

  pthread_create(&ager_tid, NULL, rcvr_thread, (void *)this);
  pthread_create(&rcvr_tid, NULL, ager_thread, (void *)this);
}

ClientWindow::~ClientWindow()
{
  pthread_mutex_lock(&lock);

  if (pixels)
  {
    free(pixels);       
    pixels = NULL;

    free(negative_pixels);
    negative_pixels = NULL;

    free(frameids);
    frameids = NULL;

    free(negative_frameids);
    negative_frameids = NULL;

    free(frame_times);
    frame_times = NULL;
  }

	if (ager_tid != (pthread_t)-1)
	{
		kill_threads = true;
		pthread_join(ager_tid, NULL);
		pthread_join(rcvr_tid, NULL);
	}

  pthread_mutex_unlock(&lock);
}

void
ClientWindow::Resize(int w, int h)
{
  pthread_mutex_lock(&lock);

  width = w;
  height = h;

  if (pixels)
  {
    free(pixels);
    free(negative_pixels);
    free(frameids);
    free(negative_frameids);
    free(frame_times);
  }

  pixels             = (float *)malloc(width*height*4*sizeof(float));
  negative_pixels    = (float *)malloc(width*height*4*sizeof(float));
  frameids           = (int *)malloc(width*height*sizeof(int));
  negative_frameids  = (int *)malloc(width*height*sizeof(int));
  frameids           = (int *)malloc(width*height*sizeof(int));
  frame_times        = (long *)malloc(width*height*sizeof(long));

  memset(frameids, 0, width*height*sizeof(int));
  memset(negative_frameids, 0, width*height*sizeof(int));

  long now = my_time();
  for (int i = 0; i < width*height; i++)
    frame_times[i] = now;

  for (int i = 0; i < 4*width*height; i++)
  {
    pixels[i] = (i & 0x3) == 0x3 ? 1.0 : 0.0;
    negative_pixels[i] = (i & 0x3) == 0x3 ? 1.0 : 0.0;
  }

  std::stringstream wndw;
  wndw << "window " << width << " " << height;

  std::string so = wndw.str();
  if (! CSendRecv(so) || so != "ok")
  {
    std::cerr << "error setting window size: " << so << "\n";
    exit(1);
  }

  pthread_mutex_unlock(&lock);

  Clear();
}

void ClientWindow::Clear()
{
  pthread_mutex_lock(&lock);

  if (pixels)
  {
    float *pix = pixels;
    for (int i = 0; i < 4*width*height; i++)
      *pix++ = 0.0;
  }

  pthread_mutex_unlock(&lock);
  pthread_mutex_destroy(&lock);
}

long 
ClientWindow::my_time()
{
  timespec s;
  clock_gettime(CLOCK_REALTIME, &s);
  return 1000000000*s.tv_sec + s.tv_nsec;
}

void *
ClientWindow::ager_thread(void *p)
{
	ClientWindow *me = (ClientWindow *)p;

	while (! me->kill_threads)
	{
		struct timespec rm, tm = {0, 100000000};
		nanosleep(&tm, &rm);
		me->FrameBufferAger();
	}

	pthread_exit(NULL);
}

void
ClientWindow::FrameBufferAger()
{
	if (pixels && frameids && frame_times)
	{
		pthread_mutex_lock(&lock);

		long now = my_time();

		for (int offset = 0; offset < width*height; offset++)
		{
			int fid = frameids[offset];
			if (fid > 0 && fid < current_frame)
			{
				long tm = frame_times[offset];
				float sec = (now - tm) / 1000000000.0;

				float *pix = pixels + (offset*4);
				if (sec > max_age)
				{
					if (sec > (max_age + fadeout))
					{
						frame_times[offset] = now;
						frameids[offset] = current_frame;
						*pix++ = 0.0, *pix++ = 0.0, *pix++ = 0.0, *pix++ = 1.0;
					}
					else
						*pix++ *= 0.9, *pix++ *= 0.9, *pix++ *= 0.9, *pix++ = 1.0;
				}
			}
		}

    pthread_mutex_unlock(&lock);
  }
}

void *
ClientWindow::rcvr_thread(void *p)
{
	ClientWindow *me = (ClientWindow *)p;

	while (!me->kill_threads)
  {
    if (me->DWait(0.1))
    {
      char *buf; int n;
      me->DRecv(buf, n);

      char *ptr = buf;
      int knt = *(int *)ptr;
      ptr += sizeof(int);
      int frame = *(int *)ptr;
      ptr += sizeof(int);
      int sndr = *(int *)ptr;
      ptr += sizeof(int);
      Pixel *p = (Pixel *)ptr;

      me->AddPixels(p, knt, frame);
  
      free(buf);
    }
  }

	pthread_exit(NULL);
}

void
ClientWindow::AddPixels(Pixel *p, int n, int frame)
{
	pthread_mutex_lock(&lock);

  // std::cerr << "n = " << n << " f = " << frame << "\n";
  // std::cerr << "  p[0]: " << p[0].x << " " << p[0].y << "\n";
  // std::cerr << "  p[n-1]: " << p[n-1].x << " " << p[n-1].y << "\n";

	long now = my_time();

	if (frame >= current_frame)
	{
		//  If frame is strictly greater than current_frame then
		//  updating it will kick the ager to begin aging
		//  any pixels from prior frames.   We want to start the 
		//  aging process from the arrival of the first contribution
		//  to the new frame rather than its updated time.
		
		if (frame > current_frame)
		{
      this_frame_pixel_count = 0;
			current_frame = frame;

			for (int offset = 0; offset < width*height; offset++)
				frame_times[offset] = now;
		}

		// Only bump current frame IF this is a positive pixel

		for (int i = 0; i < n; i++, p++, this_frame_pixel_count++)
		{
      if (p->x < 0 || p->x >= width || p->y < 0 || p->y >= height)
      {
        std::cerr << "pixel error: " << p->x << " " << p->y << "\n";
        exit(1);
      }

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

      if (frameids[offset] == frame)
			{
#if 1
				*pix++ += p->r;
				*pix++ += p->g;
				*pix++ += p->b;
#else
				float r = pix[0] + p->r;
				float g = pix[1] + p->g;
				float b = pix[2] + p->b;

				if (r < 0.01 && g < 0.01 && b < 0.01)
					std::cerr << "A last " << pix[0] << " " << pix[1] << " " << pix[2] << "\n"
										<< "  cont " << p->r << " " << p->g << " " << p->b << "\n"
										<< "  next " << r << " " << g << " " << b << "\n";

				*pix++ = r;
				*pix++ = g;
				*pix++ = b;
#endif

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
					frameids[offset] = frame;
					// frame_times[offset] = now;
					if (current_frame == negative_frameids[offset])
					{
						*pix++ = (*npix + p->r);
					  *npix = 0.0;
					  npix++;
						*pix++ = (*npix + p->g);
					  *npix = 0.0;
					  npix++;
						*pix++ = (*npix + p->b);
					  *npix = 0.0;
					  npix++;
#if 0
						if (pix[-3] < 0.01 && pix[-2] < 0.01 && pix[-1] < 0.01) 
							std::cerr << "B " << p->r << " " << p->g << " " << p->b << "\n";
#endif
					}
					else
					{
#if 0
						if (p->r < 0.01 && p->g < 0.01 && p->b < 0.01)
							std::cerr << "C last is overridden by " << p->r << " " << p->g << " " << p->b << "\n";
#endif
						*pix++ = p->r;
						*pix++ = p->g;
						*pix++ = p->b;
					}
				}
			}

      if (save_partial_updates)
      {
        if (this_frame_pixel_count >= next_partial_frame_pixel_count)
        {
          std::stringstream s;
          s << "partial-" << current_partial_frame_count << ".png";
          ImageWriter writer;
          writer.Write(width, height, pixels, s.str().c_str());

          current_partial_frame_count ++;

          if (current_partial_frame_count < (number_of_partial_frames - 1))
            next_partial_frame_pixel_count += partial_frame_pixel_delta;
          else if (current_partial_frame_count == (number_of_partial_frames - 1))
          {
            next_partial_frame_pixel_count = partial_frame_pixel_count - 2;
          }
          else
            save_partial_updates = false;
        }
      }
		}
	}

	pthread_mutex_unlock(&lock);
}

} // namespace gxy
