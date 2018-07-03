#include "AsyncRendering.h"
#include <pthread.h>
#include <time.h>

namespace pvol
{

KEYED_OBJECT_TYPE(AsyncRendering)

void
AsyncRendering::initialize()
{
  Rendering::initialize();
  current = -1;
	max_age = 1.0;

  negative_pixels = NULL;
  frameids = NULL;
  negative_frameids = NULL;
  frame_times = NULL;
  kill_ager = false;
	ager_tid = -1;
	
	t_start = my_time();

	pthread_mutex_init(&lock, NULL);
}

AsyncRendering::~AsyncRendering()
{
	if (ager_tid != -1)
	{
		kill_ager = true;
		pthread_join(ager_tid, NULL);
	}

  if (negative_pixels) free(negative_pixels);
  if (frameids) free(frameids);
  if (negative_frameids) free(negative_frameids);
  if (frame_times) free(frame_times);
}

long 
AsyncRendering::my_time()
{
  timespec s;
  clock_gettime(CLOCK_REALTIME, &s);
  return 1000000000*s.tv_sec + s.tv_nsec;
}

void *
AsyncRendering::FrameBufferAger_thread(void *p)
{
	AsyncRendering *me = (AsyncRendering *)p;

	while (! me->kill_ager)
	{
		struct timespec rm, tm = {0, 100000000};
		nanosleep(&tm, &rm);
		me->FrameBufferAger();
	}

	pthread_exit(NULL);
}

void
AsyncRendering::FrameBufferAger()
{
	if (framebuffer && frameids && frame_times)
	{
		pthread_mutex_lock(&lock);

		long now = my_time();

		for (int offset = 0; offset < width*height; offset++)
		{
			int fid = frameids[offset];
			if (fid > 0 && fid < current)
			{
				long tm = frame_times[offset];
				float sec = (now - tm) / 1000000000.0;

				float *pix = framebuffer + (offset*4);
				if (sec > max_age)
				{
					frame_times[offset] = now;
					frameids[offset] = current;
					*pix++ = 0.0, *pix++ = 0.0, *pix++ = 0.0, *pix++ = 1.0;
				}
				else
					*pix++ *= 0.9, *pix++ *= 0.9, *pix++ *= 0.9, *pix++ = 1.0;
			}
		}
		pthread_mutex_unlock(&lock);
	}
}

bool
AsyncRendering::local_commit(MPI_Comm c)
{ 
	if (super::local_commit(c))
		return true;

  if (IsLocal() && !frameids)
  {
		pthread_mutex_lock(&lock);
    frameids = new int[GetTheWidth()*GetTheHeight()];
    memset(frameids, 0, GetTheWidth()*GetTheHeight()*sizeof(int));

		negative_pixels    = (float *)malloc(GetTheWidth()*GetTheHeight()*4*sizeof(float));
		negative_frameids  = (int *)malloc(GetTheWidth()*GetTheHeight()*sizeof(int));
		frameids           = (int *)malloc(GetTheWidth()*GetTheHeight()*sizeof(int));
		frame_times        = (long *)malloc(GetTheWidth()*GetTheHeight()*sizeof(long));

		memset(frameids, 0, GetTheWidth()*GetTheHeight()*sizeof(int));
		memset(negative_frameids, 0, GetTheWidth()*GetTheHeight()*sizeof(int));

		long now = my_time();
		for (int i = 0; i < GetTheWidth()*GetTheHeight(); i++)
			frame_times[i] = now;

		for (int i = 0; i < 4*GetTheWidth()*GetTheHeight(); i++)
		{
			framebuffer[i] = (i & 0x3) == 0x3 ? 1.0 : 0.0;
			negative_pixels[i] = (i & 0x3) == 0x3 ? 1.0 : 0.0;
		}

		pthread_create(&ager_tid, NULL, FrameBufferAger_thread, (void *)this);
		pthread_mutex_unlock(&lock);
  }

  return false;
}

void
AsyncRendering::AddLocalPixels(Pixel *p, int n, int frame, int s)
{
	pthread_mutex_lock(&lock);

	if (frame >= current)
	{
		long now = my_time();

		// Only bump current frame IF this is a positive pixel

		for (int i = 0; i < n; i++, p++)
		{
			size_t offset = p->y*GetTheWidth() + p->x;
			float *pix = framebuffer + (offset<<2);
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
					current = frame;
					frameids[offset] = frame;
					frame_times[offset] = now;
					if (current == negative_frameids[offset])
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
	}

	pthread_mutex_unlock(&lock);
}

}
