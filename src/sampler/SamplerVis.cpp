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
#include "SamplerVis.h"
#include "SamplerVis_ispc.h"

#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(SamplerVis)

void
SamplerVis::Register()
{
	RegisterClass();
}

SamplerVis::~SamplerVis()
{
}

void
SamplerVis::initialize()
{
  super::initialize();
}

void 
SamplerVis::initialize_ispc()
{
  super::initialize_ispc();
  ispc::SamplerVis_initialize(ispc);
}

void
SamplerVis::allocate_ispc()
{
  ispc = ispc::SamplerVis_allocate();
}

bool 
SamplerVis::Commit(DatasetsP datasets)
{
	return Vis::Commit(datasets);
}

bool
SamplerVis::LoadFromJSON(Value& v)
{
  Vis::LoadFromJSON(v);

  if (v.HasMember("tolerance"))
     tolerance = v["tolerance"].GetDouble();

  return true;
}

void
SamplerVis::SetTheOsprayDataObject(OsprayObjectP o)
{
  super::SetTheOsprayDataObject(o);
}

int
SamplerVis::serialSize() 
{
	return super::serialSize() + sizeof(float);
}

unsigned char *
SamplerVis::deserialize(unsigned char *ptr) 
{
  ptr = super::deserialize(ptr);

  tolerance = *(float *)ptr;
  ptr += sizeof(float);

  return ptr;
}

unsigned char *
SamplerVis::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);

  *(float *)ptr = tolerance;
  ptr += sizeof(float);

  return ptr;
}

bool 
SamplerVis::local_commit(MPI_Comm c)
{
  if(super::local_commit(c))  
    return true;
  
  ispc::SamplerVis_set_tolerance(ispc, tolerance);
  return false;
}

void 
SamplerVis::SetTolerance(float t) { tolerance = t; }

float 
SamplerVis::GetTolerance() { return tolerance; }

} // namespace gxy
