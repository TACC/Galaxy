// ========================================================================== //
// Copyright (c) 2014-2020 The University of Texas at Austin.                 //
// All rights reserved.                                                       //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// A copy of the License is included with this software in the file LICENSE.  //
// If your copy does not contain the License, you may obtain a copy of the    //
// License at:                                                                //
//                                                                            //
//     https://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  //
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
//                                                                            //
// ========================================================================== //

#include "Rendering.h"

#include "Application.h"
#include "Camera.h"
#include "ImageWriter.h"
#include "KeyedObject.h"
#include "Rays.h"
#include "Renderer.h"
#include "RenderingEvents.h"
#include "RenderingSet.h"
#include "Visualization.h"
#include "Work.h"

using namespace std;

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(Rendering)

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
	frame = -1;

#ifndef GXY_WRITE_IMAGES
  kbuffer = NULL;
#endif
}

Rendering::~Rendering() 
{
#ifdef GXY_LOGGING
	APP_LOG(<< "Rendering dtor (key " << getkey() << ")");
#endif
  if (framebuffer)
	{
		// APP_LOG(<< "FB " << std::hex << framebuffer << " deleted");
		delete[] framebuffer;
	}

#ifndef GXY_WRITE_IMAGES
  if (kbuffer) delete[] kbuffer;
#endif
}

bool
Rendering::IsLocal()
{
  if (owner == -1)
  {
    cerr << "ERROR: IsLocal called before Rendering owner set" << endl;
    exit(1);
  }
  return owner == GetTheApplication()->GetRank();
}

#ifdef GXY_WRITE_IMAGES

#define ACCUMULATE_PIXEL(X, Y, R, G, B, O)                            	 \
{                                                                        \
	int offset = Y*width + X;																							 \
  float *ptr = framebuffer + (offset<<2);                                \
  *ptr++ += R;                                                           \
  *ptr++ += G;                                                           \
  *ptr++ += B;                                                           \
  *ptr++ += O;                                                           \
	accumulation_knt++;																										 \
}

#else
    
#define ACCUMULATE_PIXEL(f, X, Y, R, G, B, O)                            \
{     	                                                                 \
	int offset = Y*width + X;																							 \
  float *ptr = framebuffer + (offset<<2);                                \
	if (kbuffer[offset] < f)																							 \
	{																																			 \
		ptr[0] = 0;																													 \
		ptr[1] = 0;																													 \
		ptr[2] = 0;																													 \
		ptr[3] = 0;																													 \
		kbuffer[offset] = f;																								 \
	}																																			 \
  *ptr++ += R;                                                           \
  *ptr++ += G;                                                           \
  *ptr++ += B;                                                           \
  *ptr++ += O;                                                           \
	accumulation_knt++;																										 \
}

#endif
    
void
Rendering::AddLocalPixels(Pixel *p, int n, int f, int s)
{
#if defined(GXY_EVENT_TRACKING)
	GetTheEventTracker()->Add(new LocalPixelsEvent(n, this->getkey(), f));
#endif

  if (! framebuffer)
  {
    cerr << "ERROR: Rendering::AddLocalPixel called by non-owner" << endl;
    exit(1);
  }

	if (f >= frame)
	{
		if (f > frame)
			frame = f;
		
		while (n-- > 0)
		{
	#ifdef GXY_WRITE_IMAGES
			ACCUMULATE_PIXEL(p->x, p->y, p->r, p->g, p->b, p->o);
	#else
			ACCUMULATE_PIXEL(f, p->x, p->y, p->r, p->g, p->b, p->o);
	#endif
			p++;
		}
	}
}

vec3f scale1(float s, vec3f v) { return vec3f(s*v.x, s*v.y, s*v.z); }

void
Rendering::resolve_lights(RendererP renderer)
{
	Lighting *srcLights = GetTheVisualization()->get_the_lights();

	CameraP  theCamera = GetTheCamera();

	vec3f viewpoint, viewup, viewdir;
	theCamera->get_viewpoint(viewpoint);
	theCamera->get_viewup(viewup);
	theCamera->get_viewdirection(viewdir);
	normalize(viewup);
	normalize(viewdir);

	vec3f right, up;
	cross(viewdir, viewup, right);
	normalize(right);
	cross(viewdir, right, up);

	int n_lts, *src_t; float *src_l;
	srcLights->GetLights(n_lts, src_l, src_t);

	float *l = new float[n_lts * 3];
	int   *t = new int[n_lts];

	vec3f *il = (vec3f *)src_l;
	int   *it = src_t;
	vec3f *ol = (vec3f *)l;
	int   *ot = t;
	for (int i = 0; i < n_lts; i++)
  {
    int t = *it++;
    if (t == 1)
		{
			*ol++ = viewpoint + scale1(il->x, right) + scale1(il->y, up) + scale1(il->z, viewdir);
			il++;
			*ot++ = 2;
		}
		else
		{
			*ol++ = *il++;
			*ot++ = t;
		}
	}

	lights.SetLights(n_lts, l, t);
	delete[] l; delete[] t;

	int nao; float rao;
	srcLights->GetAO(nao, rao);
	lights.SetAO(nao, rao);

	bool shdw;
	srcLights->GetShadowFlag(shdw);
	lights.SetShadowFlag(shdw);

	float ka, kd;
	srcLights->GetK(ka, kd);
	lights.SetK(ka, kd);
}

bool
Rendering::local_commit(MPI_Comm c)
{
  if (IsLocal())
  {
    if (framebuffer)
      delete[] framebuffer;

    framebuffer = new float[width*height*4];
    memset(framebuffer, 0, width*height*4*sizeof(float));

#ifndef GXY_WRITE_IMAGES
		if (kbuffer)
			delete[] kbuffer;

    kbuffer = new int[width*height];
		memset(kbuffer, 0, width*height*sizeof(int));
#endif
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
      cerr << "ERROR: Rendering::local_reset IsLocal but no framebuffer present" << endl;
      exit(1);
    }
    for (float *p = framebuffer; p < framebuffer + width*height*4; *p++ = 0.0);

#ifndef GXY_WRITE_IMAGES
		memset(kbuffer, 0, width*height*sizeof(int));
#endif
  }
}

CameraP Rendering::GetTheCamera() { return camera; }
void Rendering::SetTheCamera(CameraP c) 
{ 
  camera = c;
  SetTheSize(c->get_width(), c->get_height());
}

DatasetsP Rendering::GetTheDatasets() { return datasets; }
void Rendering::SetTheDatasets(DatasetsP ds) { datasets = ds; }

VisualizationP Rendering::GetTheVisualization() { return visualization; }
void Rendering::SetTheVisualization(VisualizationP dm) { visualization = dm; }

void Rendering::SaveImage(string filename, int indx, bool asFloat)
{
  if ((strlen(GetTheVisualization()->GetAnnotation()) > 0) || (strlen(GetTheCamera()->GetAnnotation()) > 0))
    filename = filename + GetTheVisualization()->GetAnnotation() + GetTheCamera()->GetAnnotation();
  else
  {
    char istr[10];
    sprintf(istr, "%05d", indx);
    filename = filename + '_' + istr + GetTheVisualization()->GetAnnotation() + GetTheCamera()->GetAnnotation();
  }

  if (asFloat)
  {
    FloatImageWriter writer;
    writer.Write(width, height, framebuffer, filename.c_str());
  }
  else
  {
    ColorImageWriter writer;
    writer.Write(width, height, framebuffer, filename.c_str());
  }
}

int
Rendering::serialSize()
{
  // return KeyedObject::serialSize() + 3*sizeof(int) + 4*sizeof(Key);
  return KeyedObject::serialSize() + 3*sizeof(int) + 3*sizeof(Key);
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
  visualization = Visualization::GetByKey(*(Key *)p);
  p += sizeof(Key);
  camera = Camera::GetByKey(*(Key *)p);
  p += sizeof(Key);
  datasets = Datasets::GetByKey(*(Key *)p);
  p += sizeof(Key);
  return p;
}

} // namespace gxy
