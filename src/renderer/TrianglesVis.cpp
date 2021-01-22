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

#include "TrianglesVis.h"
#include "TrianglesVis_ispc.h"

#include <fstream>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <string>

#include "Application.h"
#include "Datasets.h"

using namespace rapidjson;

namespace gxy
{

OBJECT_CLASS_TYPE(TrianglesVis)

void
TrianglesVis::Register()
{
  RegisterClass();
}

TrianglesVis::~TrianglesVis()
{
}

void
TrianglesVis::initialize()
{
  super::initialize();
}

int 
TrianglesVis::serialSize()
{
  return super::serialSize();
}

unsigned char *
TrianglesVis::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);
  return ptr;
}

unsigned char *
TrianglesVis::deserialize(unsigned char *ptr)
{
  ptr = super::deserialize(ptr);
  return ptr;
}

bool
TrianglesVis::LoadFromJSON(Value& v)
{
  return super::LoadFromJSON(v);
}

bool
TrianglesVis::local_commit(MPI_Comm c)
{  
	return super::local_commit(c);
}

} // namespace gxy
