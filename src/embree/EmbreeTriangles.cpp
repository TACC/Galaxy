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

#include "EmbreeTriangles.h"
#include <embree3/rtcore.h>

namespace gxy 
{

KEYED_OBJECT_CLASS_TYPE(EmbreeTriangles)

void
EmbreeTriangles::Register()
{
    RegisterClass();
}

void
EmbreeTriangles::SetGeometry(GeometryP g)
{
    TrianglesP t = Triangles::Cast(g);
    if (! t)
    {
        std::cerr << "EmbreeTriangles::SetGeometry called with something other than Triangles\n";
        exit(1);
    }

    super::SetGeometry(g);
    
    int nv = t->GetNumberOfVertices();
    int nc = t->GetConnectivitySize();

    if (nv && nc)
    {
        device_geometry = rtcNewGeometry((RTCDevice)GetEmbree()->Device(), RTC_GEOMETRY_TYPE_TRIANGLE);

        rtcSetSharedGeometryBuffer(device_geometry, RTC_BUFFER_TYPE_VERTEX,
                             0, RTC_FORMAT_FLOAT3, (void *)t->GetVertices(),
                             0, 3*sizeof(float), nv);

        rtcSetSharedGeometryBuffer(device_geometry, RTC_BUFFER_TYPE_INDEX,
                             0, RTC_FORMAT_UINT3, (void *)t->GetConnectivity(),
                             0, 3*sizeof(int), nc);

        rtcCommitGeometry(device_geometry);
     }
}

}
