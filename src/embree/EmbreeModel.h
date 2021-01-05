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

#include "GalaxyObject.h"
#include "Rays.h"
#include "Embree.h"
#include "EmbreeGeometry.h"

namespace gxy {

OBJECT_POINTER_TYPES(EmbreeModel) 

class EmbreeModel : public KeyedObject
{
    KEYED_OBJECT(EmbreeModel)

public:
    virtual void initialize();
    virtual ~EmbreeModel();

    void SetEmbree(EmbreeDPtr);

    int  AddGeometry(EmbreeGeometryDPtr);
    void RemoveGeometry(int);
    void RemoveGeometry(EmbreeGeometryDPtr);

    void Intersect(RayList *);

    RTCScene GetDevice() { return scene; }

    virtual int serialSize();
    virtual unsigned char *serialize(unsigned char *);
    virtual unsigned char *deserialize(unsigned char *);

    virtual bool local_commit(MPI_Comm c);

protected:

    std::vector<EmbreeGeometryDPtr> geometries;
    std::vector<int> free_ids;

    EmbreeDPtr     embree;
    RTCScene    scene;
};

}
