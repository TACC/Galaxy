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
#include "IsoSamplerVis.h"
#include "IsoSamplerVis_ispc.h"

#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(IsoSamplerVis)

void
IsoSamplerVis::Register()
{
	RegisterClass();
}

IsoSamplerVis::~IsoSamplerVis()
{
}

void
IsoSamplerVis::initialize()
{
  super::initialize();
}

void 
IsoSamplerVis::initialize_ispc()
{
  super::initialize_ispc();
  ispc::IsoSamplerVis_initialize(ispc);
}

void
IsoSamplerVis::allocate_ispc()
{
  ispc = ispc::IsoSamplerVis_allocate();
}

bool 
IsoSamplerVis::Commit(DatasetsP datasets)
{
	return super::Commit(datasets);
}

bool
IsoSamplerVis::LoadFromJSON(Value& v)
{
  super::LoadFromJSON(v);

  if (v.HasMember("isovalue"))
     isovalue = v["isovalue"].GetDouble();

  return true;
}

void
IsoSamplerVis::SetTheOsprayDataObject(OsprayObjectP o)
{
  super::SetTheOsprayDataObject(o);
}

int
IsoSamplerVis::serialSize() 
{
	return super::serialSize() + sizeof(float);
}

unsigned char *
IsoSamplerVis::deserialize(unsigned char *ptr) 
{
  ptr = super::deserialize(ptr);

  isovalue = *(float *)ptr;
  ptr += sizeof(float);

  return ptr;
}

unsigned char *
IsoSamplerVis::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);

  *(float *)ptr = isovalue;
  ptr += sizeof(float);

  return ptr;
}

bool 
IsoSamplerVis::local_commit(MPI_Comm c)
{
  if(super::local_commit(c))  
    return true;
  
  ispc::IsoSamplerVis_set_isovalue(ispc, isovalue);
  return false;
}

void 
IsoSamplerVis::SetIsovalue(float t) { isovalue = t; }

float 
IsoSamplerVis::GetIsovalue() { return isovalue; }

} // namespace gxy
