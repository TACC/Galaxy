#include "AsyncRendering.h"

KEYED_OBJECT_TYPE(AsyncRendering)

void
AsyncRendering::initialize()
{
  Rendering::initialize();
	frameids = NULL;
  pixels = NULL;
	current = -1;
}

void
AsyncRendering::AddLocalPixels(Pixel *p, int n, int f, int s)
{
	if (!frameids || !pixels)
		return;

  if (f > current)
		current = f;

	if (f == current)
	{
		for (int i = 0; i < n; i++, p++)
		{
			// size_t offset = (((height-1)-(p->y))*GetTheWidth() + ((GetTheWidth()-1)-(p->x)));
			size_t offset = (p->y*GetTheWidth() + p->x);

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

