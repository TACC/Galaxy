#include "ServerRendering.h"
#include <pthread.h>

static pthread_mutex_t serverrendering_lock = PTHREAD_MUTEX_INITIALIZER;

KEYED_OBJECT_TYPE(ServerRendering)

void
ServerRendering::initialize()
{
  Rendering::initialize();
	max_frame = -1;
}

void
ServerRendering::AddLocalPixels(Pixel *p, int n, int f, int s)
{

	extern int debug_target;

  if (f >= max_frame)
	{
		max_frame = f;

		if (s == debug_target)
			std::cerr << "ServerRendering::AddLocalPixels from debug_target\n";
  
		char* ptrs[] = {(char *)&n, (char *)&f, (char *)&s, (char *)p};
		int   szs[] = {sizeof(int), sizeof(int), sizeof(int), static_cast<int>(n*sizeof(Pixel)), 0};

		socket->SendV(ptrs, szs);
		Rendering::AddLocalPixels(p, n, f, s);
	}
	else
	{
		if (s == debug_target)
			std::cerr << "ServerRendering::AddLocalPixels from debug_target NOT SENT\n";
	}
}

