#include "Application.h"
#include "Rendering.h"
#include "Socket.h"

namespace pvol
{

KEYED_OBJECT_POINTER(AsyncRendering)

class AsyncRendering : public Rendering
{ 
  KEYED_OBJECT_SUBCLASS(AsyncRendering, Rendering);
  
public:
	virtual void initialize();
	virtual bool local_commit(MPI_Comm);
  virtual void AddLocalPixels(Pixel *p, int n, int f, int s);

private:
	int   *frameids;
	int   current;
};
 

}
