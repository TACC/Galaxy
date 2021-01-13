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
#include "Model.h"
#include "EmbreeGeometry.h"
#include "VklVolume.h"

namespace gxy {

OBJECT_POINTER_TYPES(IntelModel) 

class IntelModel : public Model
{
    GALAXY_OBJECT_SUBCLASS(IntelModel, Model)

public:
    virtual void initialize();
    virtual ~IntelModel();

    int  AddGeometry(GeometryDPtr);
    void RemoveGeometry(int);
    int RemoveGeometry(GeometryDPtr);

    void Build();

    void Intersect(RayList *);

    virtual bool local_commit(MPI_Comm c);

protected:

    std::vector<EmbreeGeometryPtr> geometries;
    std::vector<int> free_geometry_ids;

    std::vector<VklVolumePtr> volumes;
    std::vector<int> free_volume_ids;

    RTCScene    scene;
};

}
