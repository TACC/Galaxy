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
#include "EmbreePathLines.ih" 

#include "embree3/rtcore.isph"
#include "embree3/rtcore_geometry.isph"
#include "embree3/rtcore_scene.isph"

// Force export
export void EmbreePathLines_dummy(uniform EmbreePathLines_ispc * uniform esi) {}


export void EmbreePathLines_postIntersect(uniform int gid, void *uniform _imodel, void *uniform _rayhit, void *uniform _sample)
{
  uniform IntelModel_ispc *uniform imodel = (uniform IntelModel_ispc *uniform)_imodel;
  uniform GxyRayHit *uniform rayhit = (uniform GxyRayHit *uniform)_rayhit;
  uniform GxySample *uniform sample = (uniform GxySample *uniform)_sample;

  int mask = rayhit->hit.geomID == gid;
  if (mask)
  {
    // EmbreePathLines_ispc *geom = (EmbreePathLines_ispc *)imodel->geometries[rayhit->hit.geomID];
    
    uniform unsigned int ugid = gid;
    EmbreePathLines_ispc *geom = (EmbreePathLines_ispc *)rtcGetGeometryUserData(rtcGetGeometry(imodel->scene, ugid));

    int   i0 = geom->indexCurve[rayhit->hit.primID];
    int   i1 = i0 + 1;

    float r0 = geom->vertexCurve[i0 * 4 + 3];
    float r1 = geom->vertexCurve[i1 * 4 + 3];

    float R  = ((1.f - rayhit->hit.u) * r0) + (rayhit->hit.u * r1);

    sample->normal = make_vec3f(rayhit->hit.Ng_x, rayhit->hit.Ng_y, rayhit->hit.Ng_z);

    if (geom->radius0 == geom->radius1) sample->data = geom->value0;
    else if (geom->radius0 < geom->radius1 && R < geom->radius0) sample->data = geom->value0;
    else if (geom->radius0 < geom->radius1 && R > geom->radius1) sample->data = geom->value1;
    else if (geom->radius0 > geom->radius1 && R < geom->radius1) sample->data = geom->value1;
    else if (geom->radius0 > geom->radius1 && R > geom->radius0) sample->data = geom->value0; 
    else sample->data = geom->value0 + ((R - geom->radius0) / (geom->radius1 - geom->radius0))*(geom->value1 - geom->value0);
  }

}
