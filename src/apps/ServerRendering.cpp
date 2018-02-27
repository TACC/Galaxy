#include "ServerRendering.h"

KEYED_OBJECT_TYPE(ServerRendering)

void
ServerRendering::initialize()
{
  Rendering::initialize();
}

void
ServerRendering::AddLocalPixels(Pixel *p, int n, int f, int s)
{
  if (f == GetFrame())
  {
    char* ptrs[] = {(char *)&n, (char *)&f, (char *)&s, (char *)p};
    int   szs[] = {sizeof(int), sizeof(int), sizeof(int), static_cast<int>(n*sizeof(Pixel)), 0};

    socket->SendV(ptrs, szs);
  }

  Rendering::AddLocalPixels(p, n, f, s);
}

