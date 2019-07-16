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
#include "SampleVis.h"
#include "SampleVis_ispc.h"

#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(SampleVis)

void
SampleVis::Register()
{
	RegisterClass();
}

SampleVis::~SampleVis()
{
}

void
SampleVis::initialize()
{
  super::initialize();
}

void 
SampleVis::initialize_ispc()
{
  super::initialize_ispc();
  ispc::SampleVis_initialize(ispc);
}

void
SampleVis::allocate_ispc()
{
  ispc = ispc::SampleVis_allocate();
}

bool 
SampleVis::Commit(DatasetsP datasets)
{
	return Vis::Commit(datasets);
}

bool
SampleVis::LoadFromJSON(Value& v)
{
  Vis::LoadFromJSON(v);

  if (v.HasMember("tolerance"))
     tolerance = v["tolerance"].GetDouble();

  return true;
}

void
SampleVis::SetTheOsprayDataObject(OsprayObjectP o)
{
  super::SetTheOsprayDataObject(o);
}

int
SampleVis::serialSize() 
{
	return super::serialSize() + sizeof(float);
}

unsigned char *
SampleVis::deserialize(unsigned char *ptr) 
{
  ptr = super::deserialize(ptr);

  tolerance = *(float *)ptr;
  ptr += sizeof(float);

  return ptr;
}

unsigned char *
SampleVis::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);

  *(float *)ptr = tolerance;
  ptr += sizeof(float);

  return ptr;
}

bool 
SampleVis::local_commit(MPI_Comm c)
{
  if(super::local_commit(c))  
    return true;
  
  ispc::SampleVis_set_tolerance(ispc, tolerance);
  return false;
}

void 
SampleVis::SetTolerance(float t) { tolerance = t; }

float 
SampleVis::GetTolerance() { return tolerance; }

} // namespace gxy
