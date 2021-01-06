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

#pragma once

#include "Application.h"
#include <embree3/rtcore.h>

namespace gxy { OBJECT_POINTER_TYPES(Embree) }

#include "EmbreeModel.h"

namespace gxy 
{

static void embreeError(void *ptr, enum RTCError error, const char* msg)
{
    std::cerr << "Embree error: " << error << " :: " << msg << "\n";
}

class Embree : public KeyedObject
{
    KEYED_OBJECT(Embree)

public:
    virtual ~Embree();
    virtual void initialize();

    RTCDevice Device() { return device; }
    
    virtual int serialSize();
    virtual unsigned char *serialize(unsigned char *);
    virtual unsigned char *deserialize(unsigned char *);

    virtual bool local_commit(MPI_Comm);

    EmbreeModelDPtr *NewModel();

private:

    RTCDevice   device;
};

Embree *GetEmbree();

}
