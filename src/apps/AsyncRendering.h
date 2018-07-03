#include "Application.h"
#include "Rendering.h"
#include "Socket.h"

#include <pthread.h>

namespace pvol
{

KEYED_OBJECT_POINTER(AsyncRendering)

class AsyncRendering : public Rendering
{ 
  KEYED_OBJECT_SUBCLASS(AsyncRendering, Rendering);
  
public:
	~AsyncRendering();

	virtual void initialize();
	virtual bool local_commit(MPI_Comm);
  virtual void AddLocalPixels(Pixel *p, int n, int f, int s);

	static  void *FrameBufferAger_thread(void *p);
	virtual void FrameBufferAger();

	void SetMaxAge(float f) { max_age = f; }

private:
	long	my_time();
	long  t_start;

	float				 max_age;

	float*       pixels = NULL;
	float*       negative_pixels = NULL;
	int*         frameids = NULL;
	int*         negative_frameids = NULL;
	long*        frame_times = NULL;

	pthread_t		 ager_tid;
	bool 				 kill_ager;

	int   current;

	pthread_mutex_t lock;
};
 

}
