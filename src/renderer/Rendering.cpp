#define LOGGING 0

#include "Rendering.h"

#include "Application.h"
#include "KeyedObject.h"
#include "RenderingSet.h"
#include "Camera.h"
#include "Visualization.h"
#include "Rays.h"
#include "Work.h"

#include "mypng.h"

KEYED_OBJECT_TYPE(Rendering)

void
Rendering::Register()
{
  RegisterClass();
}

void
Rendering::initialize()
{
	accumulation_knt = 0;
  width = -1;
  height = -1;
  owner = -1;
  framebuffer = NULL;
}

void
Rendering::SetTheSize(int w, int h)
{
  width = w;
  height = h;
}

Rendering::~Rendering() 
{
#if LOGGING
	APP_LOG(<< "Rendering dtor (key " << getkey() << ")");
#endif
  if (framebuffer) delete[] framebuffer;
}

bool
Rendering::IsLocal()
{
  if (owner == -1)
  {
    std::cerr << "IsLocal called before owner set\n";
    exit(0);
  }
  return owner == GetTheApplication()->GetRank();
}

#define ACCUMULATE_PIXEL(X, Y, R, G, B, O)                               \
{                                                                        \
  size_t offset = 4*((((height-1)-Y)*width + ((width-1)-X)));            \
  float *ptr = framebuffer + offset;                                     \
  *ptr++ += R;                                                           \
  *ptr++ += G;                                                           \
  *ptr++ += B;                                                           \
  *ptr++ += O;                                                           \
	accumulation_knt++;																										 \
}
    
void
Rendering::AddLocalPixels(Pixel *p, int n)
{
  if (! framebuffer)
  {
    std::cerr << "Rendering::AddLocalPixel called by non-owner\n";
    exit(0);
  }

  while (n-- > 0)
  {
    ACCUMULATE_PIXEL(p->x, p->y, p->r, p->g, p->b, p->o);
    p++;
  }
}

void
Rendering::AddLocalPixel(Pixel *r)
{
  if (framebuffer)
  {
    ACCUMULATE_PIXEL(r->x, r->y, r->r, r->g, r->b, r->o);
  }
  else
  {
    std::cerr << "Rendering::AddLocalPixel called by non-owner\n";
    exit(0);
  }
}

void
Rendering::AddLocalPixel(RayList *rl, int i)
{
  if (framebuffer)
  {
    ACCUMULATE_PIXEL(rl->get_x(i), rl->get_y(i), rl->get_r(i), rl->get_g(i), rl->get_b(i), rl->get_o(i));
  }
  else
  {
    std::cerr << "Rendering::AddLocalPixel called by non-owner\n";
    exit(0);
  }
}

bool
Rendering::local_commit(MPI_Comm c)
{
  if (IsLocal())
  {
    if (framebuffer)
      delete[] framebuffer;

    framebuffer = new float[width*height*4];
  }
  return false;
}

void
Rendering::local_reset()
{
  if (IsLocal())
  {
    if (! framebuffer)
    {
      std::cerr << "Rendering::local_reset IsLocal but no framebuffer?\n";
      exit(0);
    }
    for (float *p = framebuffer; p < framebuffer + width*height*4; *p++ = 0.0);
  }
}

Key Rendering::GetTheRenderingSetKey() { return GetTheRenderingSet()->getkey(); }
RenderingSetP Rendering::GetTheRenderingSet() { return renderingSet.lock(); }
void Rendering::SetTheRenderingSet(Key rsk) { renderingSet = RenderingSet::GetByKey(rsk); }
void Rendering::SetTheRenderingSet(RenderingSetP rs) { renderingSet = rs; }

Key Rendering::GetTheCameraKey() { return GetTheCamera()->getkey(); }
CameraP Rendering::GetTheCamera() { return camera.lock(); }
void Rendering::SetTheCamera(Key k) { camera = Camera::GetByKey(k); }
void Rendering::SetTheCamera(CameraP c) { camera = c; }

Key Rendering::GetTheDatasetsKey() { return GetTheDatasets()->getkey(); }
DatasetsP Rendering::GetTheDatasets() { return datasets.lock(); }
void Rendering::SetTheDatasets(Key k) { datasets = Datasets::GetByKey(k); }
void Rendering::SetTheDatasets(DatasetsP ds) { datasets = ds; }

Key Rendering::GetTheVisualizationKey() { return GetTheVisualization()->getkey(); }
VisualizationP Rendering::GetTheVisualization() { return visualization.lock(); }
void Rendering::SetTheVisualization(Key k) { visualization = Visualization::GetByKey(k); }
void Rendering::SetTheVisualization(VisualizationP dm) { visualization = dm; }

void Rendering::SaveImage(string filename, int indx)
{
	// std::cerr << "Accumulated " << accumulation_knt << " pixels";

  float *ptr = framebuffer;

  if ((strlen(GetTheVisualization()->GetAnnotation()) > 0) || (strlen(GetTheCamera()->GetAnnotation()) > 0))
    filename = filename + GetTheVisualization()->GetAnnotation() + GetTheCamera()->GetAnnotation() + ".png"; 
  else
  {
    char istr[10];
    sprintf(istr, "%05d", indx);
    filename = filename + '_' + istr + GetTheVisualization()->GetAnnotation() + GetTheCamera()->GetAnnotation() + ".png"; 
  }

  unsigned char *buf = new unsigned char[4*width*height];
  unsigned char *b = buf;

  for (int i = 0; i < width*height; i++)
  {
    *b++ = (unsigned char)(255*(*ptr++));
    *b++ = (unsigned char)(255*(*ptr++));
    *b++ = (unsigned char)(255*(*ptr++));
    *b++ = 0xff ; ptr ++;
  }

  write_png(filename.c_str(), width, height, (unsigned int *)buf);

  delete[] buf;
}

int
Rendering::serialSize()
{
  return KeyedObject::serialSize() + 3*sizeof(int) + 4*sizeof(Key);
}

unsigned char *
Rendering::serialize(unsigned char *p)
{
  *(int *)p = owner;
  p += sizeof(int);
  *(int *)p = width;
  p += sizeof(int);
  *(int *)p = height;
  p += sizeof(int);
  *(Key *)p = GetTheRenderingSet()->getkey();
  p += sizeof(Key);
  *(Key *)p = GetTheVisualization()->getkey();
  p += sizeof(Key);
  *(Key *)p = GetTheCamera()->getkey();
  p += sizeof(Key);
  *(Key *)p = GetTheDatasets()->getkey();
  p += sizeof(Key);
  return p;
}

unsigned char *
Rendering::deserialize(unsigned char *p)
{
  owner = *(int *)p;
  p += sizeof(int);
  width = *(int *)p;
  p += sizeof(int);
  height = *(int *)p;
  p += sizeof(int);
  renderingSet = RenderingSet::GetByKey(*(Key *)p);
  p += sizeof(Key);
  visualization = Visualization::GetByKey(*(Key *)p);
  p += sizeof(Key);
  camera = Camera::GetByKey(*(Key *)p);
  p += sizeof(Key);
  datasets = Datasets::GetByKey(*(Key *)p);
  p += sizeof(Key);
  return p;
}

