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

#include "ParticlesVis.h"
#include "ParticlesVis_ispc.h"

#include "Application.h"
#include "Datasets.h"

using namespace rapidjson;

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(ParticlesVis)

void
ParticlesVis::Register()
{
  RegisterClass();
}

ParticlesVis::~ParticlesVis()
{
	ParticlesVis::destroy_ispc();
}

void
ParticlesVis::initialize()
{
  super::initialize();

  r0 =  0.025;
  r1 =  0.0;
  v0 =  0.0;
  v1 =  0.0;
}

void
ParticlesVis::initialize_ispc()
{
  super::initialize_ispc();
  ispc::ParticlesVis_initialize(ispc);
} 
    
void
ParticlesVis::allocate_ispc()
{
  ispc = ispc::ParticlesVis_allocate();
}

int 
ParticlesVis::serialSize()
{
  return super::serialSize();
}

unsigned char *
ParticlesVis::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);
  
  return ptr;
}

unsigned char *
ParticlesVis::deserialize(unsigned char *ptr)
{
  ptr = super::deserialize(ptr);
  return ptr;
}

bool 
ParticlesVis::LoadFromJSON(Value& v)
{
  if (v.HasMember("radius0")) r0 = v["radius0"].GetDouble();
  if (v.HasMember("radius1")) r1 = v["radius1"].GetDouble();
  if (v.HasMember("value0"))  v0 = v["value0"].GetDouble();
  if (v.HasMember("value1"))  v1 = v["value1"].GetDouble();
  if (v.HasMember("radius"))
  {
    r0 = v["radius"].GetDouble();
    v0 = v1 = r1 = 0.0;
  }

  return super::LoadFromJSON(v);
}

void
ParticlesVis::destroy_ispc()
{
  if (ispc)
  {
    ispc::ParticlesVis_destroy(ispc);
  }
}

bool
ParticlesVis::local_commit(MPI_Comm c)
{  
	return super::local_commit(c);
}

void
ParticlesVis::SetTheOsprayDataObject(OsprayObjectP o)
{
  super::SetTheOsprayDataObject(o);

  ospSet1f(o->GetOSP(), "value0", v0);
  ospSet1f(o->GetOSP(), "radius0", r0);
  ospSet1f(o->GetOSP(), "value1", v1);
  ospSet1f(o->GetOSP(), "radius1", r1);
}
 
} // namespace gxy

