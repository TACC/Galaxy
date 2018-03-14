#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream>
#include "Application.h"
#include "MappedVis.h"
#include "MappedVis_ispc.h"

using namespace std;

namespace pvol
{
KEYED_OBJECT_TYPE(MappedVis)

void
MappedVis::Register()
{
	RegisterClass();
}

MappedVis::~MappedVis()
{
  if (transferFunction)
    ospRelease(transferFunction);
}

void
MappedVis::initialize()
{
	super::initialize();

  colormap.push_back(vec4f(0.0, 1.0, 0.0, 0.0));
  colormap.push_back(vec4f(1.0, 0.0, 0.0, 1.0));

  opacitymap.push_back(vec2f(0.0, 0.0));
  opacitymap.push_back(vec2f(1.0, 1.0));
  
  transferFunction = NULL;
}

void 
MappedVis::initialize_ispc()
{
	super::initialize_ispc();
  ispc::MappedVis_initialize(ispc);
}

void
MappedVis::allocate_ispc()
{
  ispc = ispc::MappedVis_allocate();
}

void 
MappedVis::Commit(DatasetsP datasets)
{
	Vis::Commit(datasets);
}

void 
MappedVis::LoadFromJSON(Value& v)
{
	Vis::LoadFromJSON(v);

  if (v.HasMember("colormap"))
  {
    colormap.clear();

    Value& cm = v["colormap"];
    for (int i = 0; i < cm.Size(); i++)
    {
      vec4f xrgb;
      xrgb.x = cm[i][0].GetDouble();
      xrgb.y = cm[i][1].GetDouble();
      xrgb.z = cm[i][2].GetDouble();
      xrgb.w = cm[i][3].GetDouble();
      colormap.push_back(xrgb);
    }
  }

  if (v.HasMember("opacitymap"))
  {
    opacitymap.clear();

    Value& om = v["opacitymap"];
    for (int i = 0; i < om.Size(); i++)
    {
      vec2f xo;
      xo.x = om[i][0].GetDouble();
      xo.y = om[i][1].GetDouble();
      opacitymap.push_back(xo);
    }
  }

}

void 
MappedVis::SaveToJSON(Value& v, Document& doc)
{
	Vis::SaveToJSON(v, doc);

  Value cmap(kArrayType);
  for (auto xrgb : colormap)
  {
    Value v(kArrayType);
    v.PushBack(Value().SetDouble(xrgb.x), doc.GetAllocator());
    v.PushBack(Value().SetDouble(xrgb.y), doc.GetAllocator());
    v.PushBack(Value().SetDouble(xrgb.z), doc.GetAllocator());
    v.PushBack(Value().SetDouble(xrgb.w), doc.GetAllocator());
    cmap.PushBack(v, doc.GetAllocator());
  }
  v.AddMember("colormap", cmap, doc.GetAllocator());

  Value omap(kArrayType);
  for (auto xo : opacitymap)
  {
    Value v(kArrayType);
    v.PushBack(Value().SetDouble(xo.x), doc.GetAllocator());
    v.PushBack(Value().SetDouble(xo.y), doc.GetAllocator());
    omap.PushBack(v, doc.GetAllocator());
  }
  v.AddMember("opacitymap", omap, doc.GetAllocator());
}

int
MappedVis::serialSize() 
{
	return super::serialSize() + sizeof(Key) +
				 sizeof(int) + colormap.size()*sizeof(vec4f) +
				 sizeof(int) + opacitymap.size()*sizeof(vec2f);
}

unsigned char *
MappedVis::deserialize(unsigned char *ptr) 
{
	ptr = super::deserialize(ptr);

  int nc = *(int *)ptr;
  ptr += sizeof(int);
  SetColorMap(nc, (vec4f *)ptr);
  ptr += nc * sizeof(vec4f);

  int no = *(int *)ptr;
  ptr += sizeof(int);
  SetOpacityMap(no, (vec2f *)ptr);
  ptr += no * sizeof(vec2f);

	return ptr;
}

unsigned char *
MappedVis::serialize(unsigned char *ptr)
{
	ptr = super::serialize(ptr);

	*(int *)ptr = colormap.size();
	ptr += sizeof(int);
	memcpy(ptr, colormap.data(), colormap.size()*sizeof(vec4f));
  //std::cerr << "DS " << ((float *)ptr)[0] << " "<< ((float *)ptr)[1] << " " << ((float *)ptr)[2] << " " << ((float *)ptr)[3] << "\n";
	ptr += colormap.size()*sizeof(vec4f);

	*(int *)ptr = opacitymap.size();
	ptr += sizeof(int);
	memcpy(ptr, opacitymap.data(), opacitymap.size()*sizeof(vec2f));
	ptr += opacitymap.size()*sizeof(vec2f);

	return ptr;
}

bool 
MappedVis::local_commit(MPI_Comm c)
{
	if(super::local_commit(c))  
    return true;
  
  if (transferFunction)
    ospRelease(transferFunction);
  
  transferFunction = ospNewTransferFunction("piecewise_linear");

  int n_colors = colormap.size();

  vec3f color[256];

  int i0 = 0, i1 = 1;
  float xmin = colormap[0].x, xmax = colormap[n_colors-1].x;
  for (int i = 0; i < 256; i++)
  {
    float x = xmin + (i / (255.0))*(xmax - xmin);
    if (x > xmax) x = xmax;

    while (colormap[i1].x < x)
      i0++, i1++;

    float d = (x - colormap[i0].x) / (colormap[i1].x - colormap[i0].x);
    color[i].x = colormap[i0].y + d * (colormap[i1].y - colormap[i0].y);
    color[i].y = colormap[i0].z + d * (colormap[i1].z - colormap[i0].z);
    color[i].z = colormap[i0].w + d * (colormap[i1].w - colormap[i0].w);
  }

  OSPData oColors = ospNewData(256, OSP_FLOAT3, color);
  ospSetData(transferFunction, "colors", oColors);
  ospRelease(oColors);
  
  //std::cerr << "CM0 " << colormap[0].x << " " << colormap[0].y << " " << colormap[0].z << " " << colormap[0].w << "\n";

  int n_opacities = opacitymap.size();
  float opacity[256];

  i0 = 0, i1 = 1;
  xmin = opacitymap[0].x, xmax = opacitymap[n_opacities-1].x;
  for (int i = 0; i < 256; i++)
  {
    float x = xmin + (i / (255.0))*(xmax - xmin);
    if (x > xmax) x = xmax;

    while (opacitymap[i1].x < x)
      i0++, i1++;

    float d = (x - opacitymap[i0].x) / (opacitymap[i1].x - opacitymap[i0].x);
    opacity[i] = opacitymap[i0].y + d * (opacitymap[i1].y - opacitymap[i0].y);
  }

  OSPData oAlphas = ospNewData(256, OSP_FLOAT, opacity);
  ospSetData(transferFunction, "opacities", oAlphas);
  ospRelease(oAlphas);

  ospSet2f(transferFunction, "valueRange", opacitymap[0].x, opacitymap[n_opacities-1].x);

  ospCommit(transferFunction);
  
  ispc::MappedVis_set_transferFunction(ispc, osp_util::GetIE(transferFunction));
  return false;
}

void 
MappedVis::SetColorMap(int n, vec4f *ptr)
{
	colormap.clear();
	for (int i = 0; i < n; i++)
		colormap.push_back(ptr[i]);
}      
			 
void 
MappedVis::SetOpacityMap(int n, vec2f *ptr)
{      
	opacitymap.clear();
	for (int i = 0; i < n; i++)
		opacitymap.push_back(ptr[i]);
}
}