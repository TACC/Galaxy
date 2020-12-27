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

#include "embree3/rtcore.isph"
#include "embree3/rtcore_geometry.isph"
#include "embree3/rtcore_ray.isph"

#include "EmbreeSpheres.ih" 

export /*unmasked*/ void EmbreeSpheres_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  uniform EmbreeSpheres_ispc *uniform self = (uniform EmbreeSpheres_ispc *uniform)args->geometryUserPtr;
  uniform RTCBounds* uniform bounds_o = args->bounds_o;
  uniform int primID = args->primID;
  uniform vec3f center = self->super.vertices[primID];
  
  uniform float radius;

  if (self->super.data && self->value0 != self->value1)
  {
    // print("A % % %\n", self->super.data, self->value0, self->value1);
    uniform float dataval = self->super.data[primID];
    uniform float d = (dataval - self->value0) / (self->value1 - self->value0);
    if (d > 1) radius = self->radius1;
    else if (d < 0) radius = self->radius0;
    else radius = self->radius0 + d*(self->radius1 - self->radius0);
  }
  else if (self->super.data)
  {
    // print("B\n");
    radius = self->super.data[primID];
  }
  else
  {
    // print("C\n");
    radius = 1;
  }

  // print("CX %\nCY %\nCZ %\nR %\n", center.x, center.y, center.z, radius);

  uniform vec3f corner = center - radius;
  bounds_o->lower_x = corner.x;
  bounds_o->lower_y = corner.y;
  bounds_o->lower_z = corner.z;

  corner = center + radius;
  bounds_o->upper_x = corner.x;
  bounds_o->upper_y = corner.y;
  bounds_o->upper_z = corner.z;

  // print("BLX %\nBLY %\nBLZ %\n", bounds_o->lower_x, bounds_o->lower_y, bounds_o->lower_z);
  // print("BUX %\nBUY %\nBUZ %\n", bounds_o->upper_x, bounds_o->upper_y, bounds_o->upper_z);
}

void EmbreeSpheres_intersect_kernel(const RTCIntersectFunctionNArguments *uniform args, const uniform bool isOcclusionTest)
{
  uniform int* uniform valid = args->valid;
  uniform EmbreeSpheres_ispc *uniform self = (uniform EmbreeSpheres_ispc *uniform)args->geometryUserPtr;
  varying RTCRayHit* uniform rayhit = (varying RTCRayHit* uniform)args->rayhit;
  varying RTCRay* uniform ray = (varying RTCRay* uniform)&rayhit->ray;
  varying RTCHit* uniform hit = (varying RTCHit* uniform)&rayhit->hit;
  uniform unsigned int primID = args->primID;

  vec3f org = make_vec3f(ray->org_x, ray->org_y, ray->org_z);
  vec3f dir = make_vec3f(ray->dir_x, ray->dir_y, ray->dir_z);
 
  if (!args->valid[programIndex]) return;

  vec3f center = self->super.vertices[primID];

  uniform float radius;
  if (self->super.data >= 0 && self->value0 != self->value1)
  {
    uniform float dataval = self->super.data[primID];
    uniform float d = (dataval - self->value0) / (self->value1 - self->value0);
    if (d > 1) radius = self->radius1;
    else if (d < 0) radius = self->radius0;
    else radius = self->radius0 + d*(self->radius1 - self->radius0);
  }
  else
  {
    radius = self->super.data[primID];
  }

  const float approxDist = dot(center - org, dir);

  const vec3f closeOrg = org + approxDist * dir;
  const vec3f A = center - closeOrg;

  const float a = dot(dir, dir);
  const float b = 2.f*dot(dir, A);
  const float c = dot(A,A)-radius*radius;

  const float radical = b*b-4.f*a*c;
  if (radical < 0.f) return;

  const float srad = sqrt(radical);

  const float t_in = (b - srad) *rcpf(2.f*a) + approxDist;
  const float t_out= (b + srad) *rcpf(2.f*a) + approxDist;

  // print("TIN %\nTOUT %\n", t_in, t_out);

  bool h = false;
  if (t_in > ray->tnear && t_in < ray->tfar) {
    h = true;
    ray->tfar = t_in;
  } else if (t_out > (ray->tnear + self->epsilon) && t_out < ray->tfar) {
    h = true;
    ray->tfar = t_out;
  }
    
  if (h) {
    hit->primID = primID;
    // OLD (third-part) VERSION OF EMBREE DOESN'T HAVE THIS! hit->geomID = args->geomID;
    if (isOcclusionTest) {
      ray->tfar = neg_inf;
    } else {
      vec3f normal = org + ray->tfar*dir - center;
      hit->Ng_x = normal.x;
      hit->Ng_y = normal.y;
      hit->Ng_z = normal.z;
    }
  }
}

export /*unmasked*/ void EmbreeSpheres_intersect(const struct RTCIntersectFunctionNArguments *uniform args)
{
  EmbreeSpheres_intersect_kernel(args,false);
}

export /*unmasked*/ void EmbreeSpheres_occluded(const struct RTCIntersectFunctionNArguments *uniform args)
{
  EmbreeSpheres_intersect_kernel(args,true);
}

// Force export
export void EmbreeSpheres_dummy(uniform EmbreeSpheres_ispc * uniform esi) {}


