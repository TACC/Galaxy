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
#include "SamplerVis.h"
#include "SamplerVis_ispc.h"
#include "IsoSamplerVis.h"
#include "GradientSamplerVis.h"

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
  IsoSamplerVis::Register();
  GradientSamplerVis::Register();
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
SamplerVis::Commit(DatasetsDPtr datasets)
{
  return Vis::Commit(datasets);
}

bool
SamplerVis::LoadFromJSON(Value& v)
{
  Vis::LoadFromJSON(v);

  return true;
}

int
SamplerVis::serialSize() 
{
  return super::serialSize();
}

unsigned char *
SamplerVis::deserialize(unsigned char *ptr) 
{
  ptr = super::deserialize(ptr);

  return ptr;
}

unsigned char *
SamplerVis::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);

  return ptr;
}

bool 
SamplerVis::local_commit(MPI_Comm c)
{
  if(super::local_commit(c))  
    return true;
  
  return false;
}

} // namespace gxy
