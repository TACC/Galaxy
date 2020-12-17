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

EmbreeTriangles::EmbreeTriangles(TrianglesP t)
{
    triangles = t;

    int nv = t->GetNumberOfVertices();
    int nc = t->GetConnectivitySize();

    if (nv && nc)
    {
        geom = rtcNewGeometry((RTCDevice)GetEmbree()->Device(), RTC_GEOMETRY_TYPE_TRIANGLE);

        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX,
                             0, RTC_FORMAT_FLOAT3, (void *)t->GetVertices(),
                             0, 3*sizeof(float), nv);

        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX,
                             0, RTC_FORMAT_UINT3, (void *)t->GetConnectivity(),
                             0, 3*sizeof(int), nc);

        rtcCommitGeometry(geom);
     }
}

}
