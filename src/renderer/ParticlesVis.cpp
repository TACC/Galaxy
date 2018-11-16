// ========================================================================== //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 //
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

OBJECT_CLASS_TYPE(ParticlesVis)

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

void 
ParticlesVis::LoadFromJSON(Value& v)
{
  super::LoadFromJSON(v);
}

void
ParticlesVis::SaveToJSON(Value& v, Document&  doc)
{
  Vis::SaveToJSON(v, doc);
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

} // namespace gxy

