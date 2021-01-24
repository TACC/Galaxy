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
#include "EmbreeGeometry_ispc.h"

#include <embree3/rtcore.h>

namespace gxy
{

OBJECT_CLASS_TYPE(EmbreeGeometry)

void
EmbreeGeometry::initialize()
{
  super::initialize();
  ((::ispc::EmbreeGeometry_ispc *)GetIspc())->device_geometry = NULL;
}

EmbreeGeometry::~EmbreeGeometry()
{
  if (GetDeviceGeometry())
    rtcReleaseGeometry(GetDeviceGeometry());
}

int EmbreeGeometry::IspcSize() { return sizeof(ispc::EmbreeGeometry_ispc); }

void
EmbreeGeometry::FinalizeData(KeyedDataObjectPtr kop)
{
    ispc::EmbreeGeometry_ispc *ispc = (ispc::EmbreeGeometry_ispc*)GetIspc();

    super::FinalizeData(kop);

    GeometryPtr geometry = Geometry::Cast(kop);
    geometry->GetLocalCounts(ispc->nv, ispc->nc);

    ispc->vertices     = (ispc::vec3f*)geometry->GetVertices();
    ispc->normals      = (ispc::vec3f*)geometry->GetNormals();
    ispc->connectivity = geometry->GetConnectivity();
    ispc->data         = geometry->GetData();
}

}
