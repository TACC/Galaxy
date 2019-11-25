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

#include "VolumeVis.h"
#include "VolumeVis_ispc.h"

#include <iostream>
#include <memory>

#include "Application.h"
#include "Datasets.h"

using namespace rapidjson;

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(VolumeVis)

void
VolumeVis::Register()
{
  RegisterClass();
}

VolumeVis::~VolumeVis()
{
  std::cerr << "XXXXXXXXXXXXXXXXXXXX VolumeVis dtor\n";
	VolumeVis::destroy_ispc();
}

void
VolumeVis::initialize()
{
  super::initialize();
  volume_rendering = false;
}

void
VolumeVis::initialize_ispc()
{
  super::initialize_ispc();
  ispc::VolumeVis_initialize(ispc);
} 
    
void
VolumeVis::allocate_ispc()
{
  ispc = ispc::VolumeVis_allocate();
}

int 
VolumeVis::serialSize()
{
  return super::serialSize() +
         sizeof(int) + slices.size()*sizeof(vec4f) +
         sizeof(int) + isovalues.size()*sizeof(float) + 
         sizeof(bool);
}

unsigned char *
VolumeVis::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);

  *(int *)ptr = slices.size();
  ptr += sizeof(int);
  memcpy(ptr, slices.data(), slices.size()*sizeof(vec4f));
  ptr += slices.size()*sizeof(vec4f);

  *(int *)ptr = isovalues.size();
  ptr += sizeof(int);
  memcpy(ptr, isovalues.data(), isovalues.size()*sizeof(float));
  ptr += isovalues.size()*sizeof(float);

  *(bool *)ptr = volume_rendering;
  ptr += sizeof(bool);

  return ptr;
}

unsigned char *
VolumeVis::deserialize(unsigned char *ptr)
{
  ptr = super::deserialize(ptr);

  int ns = *(int *)ptr;
  ptr += sizeof(int);
  SetSlices(ns, (vec4f*)ptr);
  ptr += ns*sizeof(vec4f);
  int ni = *(int *)ptr;
  ptr += sizeof(int);
  SetIsovalues(ni, (float*)ptr);
  ptr += ni*sizeof(float);
  SetVolumeRendering(*(bool *)ptr);
  ptr += sizeof(bool);
  
  return ptr;
}

bool 
VolumeVis::LoadFromJSON(Value& v)
{
  MappedVis::LoadFromJSON(v);

  isovalues.clear();
  if (v.HasMember("isovalues"))
  {
    Value& iv = v["isovalues"];
    for (int i = 0; i < iv.Size(); i++)
      isovalues.push_back(iv[i].GetDouble());
  }
      
  slices.clear();
  if (v.HasMember("slices"))
  {
    Value& sl = v["slices"];
    for (int i = 0; i < sl.Size(); i++)
    {
      vec4f xrgb;
      xrgb.x = sl[i][0].GetDouble();
      xrgb.y = sl[i][1].GetDouble();
      xrgb.z = sl[i][2].GetDouble();
      xrgb.w = sl[i][3].GetDouble();
      slices.push_back(xrgb);
    }
  }
	else if (v.HasMember("plane"))
	{
    Value& pl = v["plane"];
		vec4f xrgb;
		xrgb.x = pl[0].GetDouble();
		xrgb.y = pl[1].GetDouble();
		xrgb.z = pl[2].GetDouble();
		xrgb.w = pl[3].GetDouble();
		slices.push_back(xrgb);
	}
      
	if (v.HasMember("volume rendering"))
    SetVolumeRendering(v["volume rendering"].GetBool());
  else
    SetVolumeRendering(false);

  return true;
}

void
VolumeVis::destroy_ispc()
{
  if (ispc)
  {
    ispc::VolumeVis_destroy(ispc);
  }
}

bool
VolumeVis::local_commit(MPI_Comm c)
{  
	if (super::local_commit(c))
    return true;

	ispc::VolumeVis_SetSlices(GetIspc(), slices.size(), ((float *)slices.data()));
	ispc::VolumeVis_SetIsovalues(GetIspc(), isovalues.size(), ((float *)isovalues.data()));
	ispc::VolumeVis_SetVolumeRenderFlag(GetIspc(), volume_rendering);

	return false;
}

} // namespace gxy

