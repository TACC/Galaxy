#include "Application.h"
#include "Rendering.h"
#include "Socket.h"

namespace pvol
{

KEYED_OBJECT_POINTER(ServerRendering)

class ServerRendering : public Rendering
{ 
  KEYED_OBJECT_SUBCLASS(ServerRendering, Rendering);
  
public:
	virtual void initialize();
  virtual void AddLocalPixels(Pixel *p, int n, int f, int s);

	void SetSocket(Socket *s) { socket = s; }

private:
	Socket *socket;
	int max_frame;
};
 
}
