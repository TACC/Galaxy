// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
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

#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream>
#include "Application.h"
#include "MappedVis.h"
#include "MappedVis_ispc.h"

#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(MappedVis)

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

bool 
MappedVis::Commit(DatasetsP datasets)
{
	return Vis::Commit(datasets);
}

bool
MappedVis::LoadFromJSON(Value& v)
{
	Vis::LoadFromJSON(v);

/*
	if (v.HasMember("data range"))
    {
        data_range_min = v["data range"][0].GetDouble();
        data_range_max = v["data range"][1].GetDouble();
        data_range = true;
    }
    else
        data_range = false;
*/
           
	if (v.HasMember("transfer function"))
	{
    string fname = v["transfer function"].GetString();

	  ifstream ifs(fname);
    if (! ifs)
    {
      std::cerr << "unable to open transfer function file: " << fname << "\n";
      set_error(1);
      return false;
    }
    
		stringstream ss;
		ss << ifs.rdbuf();

		string s(ss.str().substr(ss.str().find('[')+1, ss.str().rfind(']') - 2));

		Document doc;
    if (doc.Parse<0>(ss.str().c_str()).HasParseError())
    {
      std::cerr << "JSON parse error in " << fname << "\n";
      set_error(1);
      return false;
    }

		doc.Parse(s.c_str());

		opacitymap.clear();

		Value& oa = doc["Points"];
		for (int i = 0; i < oa.Size(); i += 4)
		{
			vec2f xo;
      xo.x = oa[i+0].GetDouble();
      xo.y = oa[i+1].GetDouble();
      opacitymap.push_back(xo);
		}

		colormap.clear();

		Value& rgba = doc["RGBPoints"];
		for (int i = 0; i < rgba.Size(); i += 4)
		{
      vec4f xrgb;
      xrgb.x = rgba[i+0].GetDouble();
      xrgb.y = rgba[i+1].GetDouble();
      xrgb.z = rgba[i+2].GetDouble();
      xrgb.w = rgba[i+3].GetDouble();
      colormap.push_back(xrgb);
		}
	}
	else
	{
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

  return true;
}

void
MappedVis::SetTheOsprayDataObject(OsprayObjectP o)
{
  super::SetTheOsprayDataObject(o);

  ospSetObject(o->GetOSP(), "transferFunction", transferFunction);
  ospCommit(o->GetOSP());
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
/*
  if (data_range)
      ospSet2f(transferFunction, "valueRange", data_range_min, data_range_max);
  else
      ospSet2f(transferFunction, "valueRange", colormap[0].x, colormap[n_colors-1].x);
*/
  ospSet2f(transferFunction, "valueRange", colormap[0].x, colormap[n_colors-1].x);
  ospCommit(transferFunction);
  
  ispc::MappedVis_set_transferFunction(ispc, ospray_util::GetIE(transferFunction));
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

void
MappedVis::ScaleMaps(float xmin, float xmax)
{
  float x0 = colormap[0].x;
  float x1 = colormap[colormap.size()-1].x;

  for (auto i = 0; i < colormap.size(); i++)
    colormap[i].x = xmin + ((colormap[i].x - x0)/(x1 - x0)) * (xmax - xmin);

  x0 = opacitymap[0].x;
  x1 = opacitymap[opacitymap.size()-1].x;

  for (auto i = 0; i < opacitymap.size(); i++)
    opacitymap[i].x = xmin + ((opacitymap[i].x - x0)/(x1 - x0)) * (xmax - xmin);
}


} // namespace gxy
