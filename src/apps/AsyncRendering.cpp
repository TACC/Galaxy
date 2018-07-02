#include "AsyncRendering.h"

namespace pvol
{

KEYED_OBJECT_TYPE(AsyncRendering)

void
AsyncRendering::initialize()
{
  Rendering::initialize();
  frameids = NULL;
  current = -1;

	pthread_mutex_init(&lock, PTHREAD_MUTEX_INITIALIZER);
	pthread_mutex_lock(&lock);
}

long 
AsyncRendering::my_time()
{
  timespec s;
  clock_gettime(CLOCK_REALTIME, &s);
  return 1000000000*s.tv_sec + s.tv_nsec;
}

void *
AsyncRendering::ager_thread(void *p)
{
	AsyncRendering *me = (AsyncRendering *)p;
	while (true)
		me->age();
}

void AsyncRendering::age()
{

	pthread_mutex_lock(&lock);

	if (pixels && frameids && frame_times)
	{
		negative_frameids  = (int *)malloc(GetTheWidth()*GetTheHeight()*sizeof(int));
		frameids           = (int *)malloc(GetTheWidth()*GetTheHeight()*sizeof(int));
		frame_times        = (long *)malloc(GetTheWidth()*GetTheHeight()*sizeof(long));

		long now = my_time();

		float *pix = pixels;
		int *ids = frameids;
		long *tm = frame_times;
		for (int i = 0; i < width*height; i++, tm++)
			if (*ids++ < max_f)
			{
				float sec = (now - *tm) / 1000000000.0;
				float d = (sec > 10) ? 0.0 : (10.0 - sec) / 10.0;
				d = 0.999;
				*pix++ *= d, *pix++ *= d, *pix++ *= d, *pix++ = 1.0;
			}
			else
				pix += 4;
	}

	pthread_mutex_unlock(&lock);
}

bool
AsyncRendering::local_commit(MPI_Comm c)
{ 
  if (IsLocal())
  {
		pthread_mutex_lock(&lock);

    if (frameids)
      delete[] frameids;

    frameids = new int[GetTheWidth()*GetTheHeight()];
    memset(frameids, 0, GetTheWidth()*GetTheHeight()*sizeof(int));

		negative_pixels    = (float *)malloc(GetTheWidth()*GetTheHeight()*4*sizeof(float));
		pixels             = (float *)malloc(GetTheWidth()*GetTheHeight()*4*sizeof(float));
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
			pixels[i] = (i & 0x3) == 0x3 ? 1.0 : 0.0;
			negative_pixels[i] = (i & 0x3) == 0x3 ? 1.0 : 0.0;
		}

		pthread_mutex_unlock(&lock);
  }

  return Rendering::local_commit(c);
}

void
AsyncRendering::AddLocalPixels(Pixel *p, int n, int f, int s)
{
	if (frame >= current)
	{
		long now = my_time();

		for (int i = 0; i < n; i++, p++)
		{
			size_t offset = ((GetTheHeight()-1)-p->y)*GetTheWidth() + p->x;
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
					frameids[offset] = frame;
					*pix++ = (*npix++ + p->r);
					*pix++ = (*npix++ + p->g);
					*pix++ = (*npix++ + p->b);
				}
			}
		}
	}
}

}
