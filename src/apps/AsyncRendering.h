#include "Application.h"
#include "Rendering.h"
#include "Socket.h"

KEYED_OBJECT_POINTER(AsyncRendering)

class AsyncRendering : public Rendering
{ 
  KEYED_OBJECT_SUBCLASS(AsyncRendering, Rendering);
  
public:
	virtual void initialize();
  virtual void AddLocalPixels(Pixel *p, int n, int f, int s);

	void SetBuffers(float *p, int *f) { pixels = p; frameids = f; }

private:
	float *pixels;
	int   *frameids;
	int   current;
};
 



