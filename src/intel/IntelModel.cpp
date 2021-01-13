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

#include "Application.h"
#include "IntelModel.h"

#include <embree3/rtcore_scene.h>

#include "IntelModel_ispc.h"

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(IntelModel)

void
IntelModel::initialize()
{
    super::initialize();
}

IntelModel::~IntelModel()
{
    rtcReleaseScene(scene);
}

int
IntelModel::AddGeometry(GeometryDPtr g)
{
    int id = super::AddGeometry(g);
    EmbreeGeometryPtr eg = EmbreeGeometry::Cast(g->GetTheDeviceEquivalent());
    rtcAttachGeometryByID(scene, eg->GetDeviceGeometry(), id);
    return id;
}

int
IntelModel::RemoveGeometry(GeometryDPtr g)
{
    int id = super::RemoveGeometry(g);
    if (id != -1)
        rtcDetachGeometry(scene, id);
    return id;
}

void
IntelModel::RemoveGeometry(int id)
{
    rtcDetachGeometry(scene, id);
}

void
IntelModel::Intersect(RayList *rays)
{
    void** geoms = new void*[geometries.size()];

    for (int i = 0; i < geometries.size(); i++)
        geoms[i] = geometries[i] ? geometries[i]->GetIspc() : NULL;

    ispc::IntelModel_Intersect(scene, geoms, rays->GetRayCount(), rays->GetIspc());

    delete[] geoms;
}

}
