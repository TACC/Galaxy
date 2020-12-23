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

#include "EmbreeSpheres.h"
#include <embree3/rtcore.h>

#include "EmbreeSpheres_ispc.h"

namespace gxy 
{

EmbreeSpheres::EmbreeSpheres() : EmbreeGeometry()
{
}

void
EmbreeSpheres::CreateIspc()
{
    if (! ispc)
        ispc = malloc(sizeof(ispc::EmbreeSpheres_ispc));

    EmbreeGeometry::CreateIspc();

    ispc::EmbreeSpheres_ispc *iptr = (ispc::EmbreeSpheres_ispc*)ispc;

    iptr->radius0 = 0.0;
    iptr->value0  = 0.0;
    iptr->radius1 = 0.0;
    iptr->value1  = 0.0;
}

void
EmbreeSpheres::SetMap(float v0, float r0, float v1, float r1)
{
    ispc::EmbreeSpheres_ispc *iptr = (ispc::EmbreeSpheres_ispc*)GetIspc();

    iptr->radius0 = r0;
    iptr->value0  = v0;
    iptr->radius1 = r1;
    iptr->value1  = v1;
}

void 
EmbreeSpheres::FinalizeIspc()
{
    EmbreeGeometry::FinalizeIspc();

    ispc::EmbreeSpheres_ispc *iptr = (ispc::EmbreeSpheres_ispc*)GetIspc();

    ParticlesP p = Particles::Cast(geometry);
    if (! p)
    {
        std::cerr << "EmbreeSpheres::FinalizeIspc called with something other than Particles\n";
        exit(1);
    }

    device_geometry = rtcNewGeometry((RTCDevice)GetEmbree()->Device(), RTC_GEOMETRY_TYPE_USER);

    rtcSetGeometryUserData(device_geometry, GetIspc());
    rtcSetGeometryUserPrimitiveCount(device_geometry, p->GetNumberOfVertices());
    rtcSetGeometryBoundsFunction(device_geometry, (RTCBoundsFunction)&ispc::EmbreeSpheres_bounds, GetIspc());
    rtcSetGeometryIntersectFunction (device_geometry, (RTCIntersectFunctionN)&ispc::EmbreeSpheres_intersect);
    rtcSetGeometryOccludedFunction(device_geometry, (RTCOccludedFunctionN)&ispc::EmbreeSpheres_occluded);

    rtcCommitGeometry(device_geometry);
}

}
