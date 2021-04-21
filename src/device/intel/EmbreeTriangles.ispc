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

#include "IntelModel.ih"
#include "EmbreeTriangles.ih"

#include "embree3/rtcore_config.h"
#include "embree3/rtcore_ray.isph"
#include "embree3/rtcore_scene.isph"
#include "embree3/rtcore_geometry.isph"

export void EmbreeTriangles_postIntersect(uniform int gid, void *uniform _imodel, void *uniform _rayhit, void *uniform _sample)
{
  uniform IntelModel_ispc *uniform imodel = (uniform IntelModel_ispc *uniform)_imodel;
  uniform GxyRayHit *uniform rayhit = (uniform GxyRayHit *uniform)_rayhit;
  uniform GxySample *uniform sample = (uniform GxySample *uniform)_sample;

  uniform unsigned int ugid = gid;

  int mask = rayhit->hit.geomID == gid;
  if (mask)
  {
    // print("TPI mask %\n", mask);
    // print("TPI pid %\n", rayhit->hit.primID);
    // print("TPI gid %\n", rayhit->hit.geomID);

    EmbreeTriangles_ispc *geom = (EmbreeTriangles_ispc *)rtcGetGeometryUserData(rtcGetGeometry(imodel->scene, ugid));
    // EmbreeTriangles_ispc *geom = (EmbreeTriangles_ispc *)imodel->geometries[rayhit->hit.geomID];

    int pid = rayhit->hit.primID;
    vec3i pqr = ((vec3i *)geom->super.connectivity)[pid];

    vec3f bary = make_vec3f(1.0f - rayhit->hit.u - rayhit->hit.v, rayhit->hit.u, rayhit->hit.v);

    vec3f nx = geom->super.normals[pqr.x];
    vec3f ny = geom->super.normals[pqr.y];
    vec3f nz = geom->super.normals[pqr.z];
    sample->normal = bary.x * nx + bary.y * ny + bary.z * nz;

    float dx = geom->super.data[pqr.x];
    float dy = geom->super.data[pqr.y];
    float dz = geom->super.data[pqr.z];
    sample->data = bary.x * dx + bary.y * dy + bary.z * dz;
  }
}
