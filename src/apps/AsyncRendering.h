#include "Application.h"
#include "Rendering.h"
#include "Socket.h"

pthread_mutex_t lock;

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
	long	my_time();

	float*       pixels = NULL;
	float*       negative_pixels = NULL;
	int*         frameids = NULL;
	int*         negative_frameids = NULL;
	long*        frame_times = NULL;

	int   current;

	pthread_mutex_t lock;
};
 

}
