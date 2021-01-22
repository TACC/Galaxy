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

#include "Vkl.h"

namespace gxy
{

OBJECT_CLASS_TYPE(Vkl)

static Vkl *theVkl;
Vkl *GetVkl() { return theVkl; }

void
Vkl::Register()
{
    RegisterClass();
}

Vkl::~Vkl()
{
    vklShutdown();
}

void
Vkl::initialize()
{
    super::initialize();

    device = vklNewDriver("ispc");
    vklDriverSetErrorCallback(device, vklError, (void *)this);

    theVkl = this;
}

int
Vkl::serialSize()
{
    return super::serialSize();
}

unsigned char *
Vkl::serialize(unsigned char *p)
{
    p = super::serialize(p);
    return p;
}

unsigned char *
Vkl::deserialize(unsigned char *p)
{
    p = super::deserialize(p);
    return p;
}

bool 
Vkl::local_commit(MPI_Comm c)
{
    return false;
}

}
