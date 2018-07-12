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

#include <string>
#include <string.h>
#include <memory.h>
#include <vector>

#include <ospray/ospray.h>

#include "OSPRayObject.h"
#include "Geometry.h"
#include "Volume.h"

using namespace std;

namespace gxy
{
KEYED_OBJECT_TYPE(OSPRayObject)

void
OSPRayObject::initialize()
{
	theOSPRayObject = NULL;
  super::initialize();
}

OSPRayObject::~OSPRayObject()
{
	if (theOSPRayObject)
		ospRelease((OSPObject)theOSPRayObject);
}

void
OSPRayObject::Register()
{
	RegisterClass();
	Volume::Register();
	Geometry::Register();
}

}