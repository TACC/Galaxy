#include "AsyncRendering.h"

KEYED_OBJECT_TYPE(AsyncRendering)

void
AsyncRendering::initialize()
{
  Rendering::initialize();
	frameids = NULL;
	current = -1;
}

bool
AsyncRendering::local_commit(MPI_Comm c)
{ 
	if (IsLocal())
	{
		if (frameids)
			delete[] frameids;

		frameids = new int[GetTheWidth()*GetTheHeight()];
		memset(frameids, 0, GetTheWidth()*GetTheHeight()*sizeof(int));
	}

	return Rendering::local_commit(c);
}

void
AsyncRendering::AddLocalPixels(Pixel *p, int n, int f, int s)
{
	if (!frameids || !GetPixels())
		return;

  if (f > current)
	{
		current = f;
    float *p = GetPixels();
		int *ids = frameids;
    for (int i = 0; i < GetTheWidth()*GetTheHeight(); i++) 
    {
			if (*ids++ < (current-1))
				*p++ = 0, *p++ = 0, *p++ = 0, *p++ = 0;
			else
				p += 4;
		}
	}

	if (f == current)
	{
		for (int i = 0; i < n; i++, p++)
		{
			size_t offset = (p->y*GetTheWidth() + p->x);

			float *pix = GetPixels() + (offset<<2);
			if (frameids[offset] < frame)
			{
				pix[0] = 0;
				pix[1] = 0;
				pix[2] = 0;
				pix[3] = 0;
				frameids[offset] = frame;
			}
			*pix++ += p->r;
			*pix++ += p->g;
			*pix++ += p->b;
			*pix++ += p->o;
		}
	}
}

