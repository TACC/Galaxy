#include "Application.h"
#include "Renderer.h"

KEYED_OBJECT_POINTER(ServerRendering)

class ServerRendering : public Rendering
{ 
  KEYED_OBJECT_SUBCLASS(ServerRendering, Rendering);
  
public:
	virtual void initialize();
  virtual void AddLocalPixels(Pixel *p, int n, int f);

};
 



