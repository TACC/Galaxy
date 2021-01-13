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

#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream>
#include "Application.h"
#include "GradientSamplerVis.h"
#include "GradientSamplerVis_ispc.h"

#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(GradientSamplerVis)

void
GradientSamplerVis::Register()
{
	RegisterClass();
}

GradientSamplerVis::~GradientSamplerVis()
{
}

void
GradientSamplerVis::initialize()
{
  super::initialize();
}

void 
GradientSamplerVis::initialize_ispc()
{
  super::initialize_ispc();
  ispc::GradientSamplerVis_initialize(ispc);
}

void
GradientSamplerVis::allocate_ispc()
{
  ispc = ispc::GradientSamplerVis_allocate();
}

bool 
GradientSamplerVis::Commit(DatasetsDPtr datasets)
{
	return super::Commit(datasets);
}

bool
GradientSamplerVis::LoadFromJSON(Value& v)
{
  super::LoadFromJSON(v);

  if (v.HasMember("tolerance"))
     tolerance = v["tolerance"].GetDouble();

  return true;
}

int
GradientSamplerVis::serialSize() 
{
	return super::serialSize() + sizeof(float);
}

unsigned char *
GradientSamplerVis::deserialize(unsigned char *ptr) 
{
  ptr = super::deserialize(ptr);

  tolerance = *(float *)ptr;
  ptr += sizeof(float);

  return ptr;
}

unsigned char *
GradientSamplerVis::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);

  *(float *)ptr = tolerance;
  ptr += sizeof(float);

  return ptr;
}

bool 
GradientSamplerVis::local_commit(MPI_Comm c)
{
  if(super::local_commit(c))  
    return true;
  
  ispc::GradientSamplerVis_set_tolerance(ispc, tolerance);
  return false;
}

void 
GradientSamplerVis::SetTolerance(float t) { tolerance = t; }

float 
GradientSamplerVis::GetTolerance() { return tolerance; }

} // namespace gxy
