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

#include "PointSurfaceVis.h"
#include "PointSurfaceVis_ispc.h"
#include "OsprayPointSurface.h"

#include "Application.h"
#include "Datasets.h"

using namespace rapidjson;

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(PointSurfaceVis)

void
PointSurfaceVis::Register()
{
  RegisterClass();
}

PointSurfaceVis::~PointSurfaceVis()
{
	PointSurfaceVis::destroy_ispc();
}

void
PointSurfaceVis::initialize()
{
  super::initialize();

  r0 =  1.0;
  r1 =  2.0;
  dr =  1.0;
}

void
PointSurfaceVis::initialize_ispc()
{
  super::initialize_ispc();
  ispc::PointSurfaceVis_initialize(ispc);
} 
    
void
PointSurfaceVis::allocate_ispc()
{
  ispc = ispc::PointSurfaceVis_allocate();
}

int
PointSurfaceVis::serialSize()
{
  return super::serialSize() + 3 * sizeof(float);
}

unsigned char *
PointSurfaceVis::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);

  *(float *)ptr = r0; ptr += sizeof(float);
  *(float *)ptr = r1; ptr += sizeof(float);
  *(float *)ptr = dr; ptr += sizeof(float);
 
  return ptr;
}

unsigned char *
PointSurfaceVis::deserialize(unsigned char *ptr)
{
  ptr = super::deserialize(ptr);

  r0 = *(float *)ptr; ptr += sizeof(float);
  r1 = *(float *)ptr; ptr += sizeof(float);
  dr = *(float *)ptr; ptr += sizeof(float);

  return ptr;
}


bool 
PointSurfaceVis::LoadFromJSON(Value& v)
{
  if (v.HasMember("r0")) r0 = v["r0"].GetDouble();
  if (v.HasMember("r1")) r1 = v["r1"].GetDouble();
  if (v.HasMember("dr")) dr = v["dr"].GetDouble();

  return super::LoadFromJSON(v);
}

void
PointSurfaceVis::destroy_ispc()
{
  if (ispc)
  {
    ispc::PointSurfaceVis_destroy(ispc);
  }
}

bool
PointSurfaceVis::local_commit(MPI_Comm c)
{  
	return super::local_commit(c);
}

void
PointSurfaceVis::SetTheOsprayDataObject(OsprayObjectP o)
{
  super::SetTheOsprayDataObject(o);

  ospSet1f(o->GetOSP(), "r0", r0);
  ospSet1f(o->GetOSP(), "r1", r1);
  ospSet1f(o->GetOSP(), "dr", dr);
}

OsprayObjectP
PointSurfaceVis::CreateTheOsprayDataObject(KeyedDataObjectP kdop)
{
  OsprayObjectP ospData = OsprayObject::Cast(OsprayPointSurface::NewP(Particles::Cast(kdop)));
  return ospData;
}
 
} // namespace gxy

