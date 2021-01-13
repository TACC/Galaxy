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

#include "IntelDevice.h"
#include "EmbreeTriangles.h"
#include "EmbreeGeometry.h"
#include "EmbreeGeometry_ispc.h"
#include <embree3/rtcore.h>

namespace gxy 
{

EmbreeTriangles::EmbreeTriangles() : EmbreeGeometry()
{
}

void
EmbreeTriangles::FinalizeIspc()
{
    EmbreeGeometry::FinalizeIspc();

    TrianglesDPtr t = Triangles::DCast(geometry);
    if (! t)
    {
        std::cerr << "EmbreeTriangles::FinalizeIspc called with something other than Triangles\n";
        exit(1);
    }

    int nv = t->GetNumberOfVertices();
    int nc = t->GetConnectivitySize();

    ispc::EmbreeGeometry_ispc *iptr = (ispc::EmbreeGeometry_ispc *)GetIspc();
    if (nv && nc)
    {
        IntelDevicePtr intel_device = IntelDevice::Cast(GetTheDevice());

        device_geometry = rtcNewGeometry(intel_device->get_embree(), RTC_GEOMETRY_TYPE_TRIANGLE);

        rtcSetSharedGeometryBuffer(device_geometry, RTC_BUFFER_TYPE_VERTEX,
                             0, RTC_FORMAT_FLOAT3, (void *)iptr->vertices,
                             0, 3*sizeof(float), nv);

        rtcSetSharedGeometryBuffer(device_geometry, RTC_BUFFER_TYPE_INDEX,
                             0, RTC_FORMAT_UINT3, (void *)iptr->connectivity,
                             0, 3*sizeof(int), nc);

        rtcCommitGeometry(device_geometry);
     }
}

}
