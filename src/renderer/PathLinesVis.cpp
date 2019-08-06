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

#include "PathLinesVis.h"
#include "PathLinesVis_ispc.h"

#include "Application.h"
#include "Datasets.h"

using namespace rapidjson;

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(PathLinesVis)

void
PathLinesVis::Register()
{
  RegisterClass();
}

PathLinesVis::~PathLinesVis()
{
	PathLinesVis::destroy_ispc();
}

void
PathLinesVis::initialize()
{
  super::initialize();

  r0 = -1.0;
  r1 =  1.0;
  v0 =  0.0;
  v1 =  1.0;
}

void
PathLinesVis::initialize_ispc()
{
  super::initialize_ispc();
  ispc::PathLinesVis_initialize(ispc);
} 
    
void
PathLinesVis::allocate_ispc()
{
  ispc = ispc::PathLinesVis_allocate();
}

int 
PathLinesVis::serialSize()
{
  return super::serialSize() + 4 * sizeof(float);
}

unsigned char *
PathLinesVis::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);

  *(float *)ptr = v0; ptr += sizeof(float);
  *(float *)ptr = r0; ptr += sizeof(float);
  *(float *)ptr = v1; ptr += sizeof(float);
  *(float *)ptr = r1; ptr += sizeof(float);
  
  return ptr;
}

unsigned char *
PathLinesVis::deserialize(unsigned char *ptr)
{
  ptr = super::deserialize(ptr);

  v0 = *(float *)ptr; ptr += sizeof(float);
  r0 = *(float *)ptr; ptr += sizeof(float);
  v1 = *(float *)ptr; ptr += sizeof(float);
  r1 = *(float *)ptr; ptr += sizeof(float);

  
  return ptr;
}

bool 
PathLinesVis::LoadFromJSON(Value& v)
{
  if (v.HasMember("radius0")) r0 = v["radius0"].GetDouble();
  if (v.HasMember("radius1")) r1 = v["radius1"].GetDouble();
  if (v.HasMember("value0"))  v0 = v["value0"].GetDouble();
  if (v.HasMember("value1"))  v1 = v["value1"].GetDouble();

  return super::LoadFromJSON(v);
}

void
PathLinesVis::destroy_ispc()
{
  if (ispc)
  {
    ispc::PathLinesVis_destroy(ispc);
  }
}

bool
PathLinesVis::local_commit(MPI_Comm c)
{  
	return super::local_commit(c);
}

void
PathLinesVis::SetTheOsprayDataObject(OsprayObjectP o)
{
  super::SetTheOsprayDataObject(o);

  ospSet1f(o->GetOSP(), "value0", v0);
  ospSet1f(o->GetOSP(), "radius0", r0);

  ospSet1f(o->GetOSP(), "value1", v1);
  ospSet1f(o->GetOSP(), "radius1", r1);
}

void
PathLinesVis::ScaleMaps(float xmin, float xmax)
{
  super::ScaleMaps(xmin, xmax);
  v0 = xmin;
  v1 = xmax;
}

 
} // namespace gxy

