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

#include <embree3/rtcore.h>
#include <iostream>

#include "Embree.h"
#include "Embree_ispc.h"

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(Embree)

static Embree *theEmbree;
Embree *GetEmbree() { return theEmbree; }

void
Embree::Register()
{
    RegisterClass();
}

Embree::~Embree()
{
    rtcReleaseDevice(device);
}

void
Embree::initialize()
{
    super::initialize();

    device = rtcNewDevice(NULL);
    rtcSetDeviceErrorFunction(device, embreeError, (void *)this);

    theEmbree = this;
}

int
Embree::serialSize()
{
    return super::serialSize();
}

unsigned char *
Embree::serialize(unsigned char *p)
{
    p = super::serialize(p);
    return p;
}

unsigned char *
Embree::deserialize(unsigned char *p)
{
    p = super::deserialize(p);
    return p;
}

bool 
Embree::local_commit(MPI_Comm c)
{
    return false;
}

void
Embree::Intersect(EmbreeModelP emp, int n, RayList* rays)
{
    ispc::Embree_Intersect(emp->GetDevice(), n, rays->GetIspc());
}

}
