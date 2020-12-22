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

#include "Geometry.h"
#include "EmbreeGeometry.h"

#include <embree3/rtcore.h>

namespace gxy
{

EmbreeGeometry::EmbreeGeometry()
{
    geometry = NULL;
    device_geometry = NULL;
    ispc = NULL;
}

EmbreeGeometry::~EmbreeGeometry()
{
    if (device_geometry) 
    {
        rtcReleaseGeometry(device_geometry);
        free(ispc);
        ispc = NULL;
    }
}

void
EmbreeGeometry::SetGeometry(GeometryP g)
{
    geometry = g;
}

void
EmbreeGeometry::CreateIspc()
{
    if (! ispc)
        ispc = malloc(sizeof(ispc::EmbreeGeometry_ispc));
}

void
EmbreeGeometry::FinalizeIspc()
{
    ispc::EmbreeGeometry_ispc *iptr = (ispc::EmbreeGeometry_ispc*)ispc;

    geometry->GetLocalCounts(iptr->nv, iptr->nc);

    iptr->vertices     = (ispc::vec3f*)geometry->GetVertices();
    iptr->normals      = (ispc::vec3f*)geometry->GetNormals();
    iptr->connectivity = geometry->GetConnectivity();
    iptr->data         = geometry->GetData();
}

GeometryP 
EmbreeGeometry::GetGeometry()
{
    return geometry;
}

}
