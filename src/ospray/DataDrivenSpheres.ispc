// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// ospray
#include "math/vec.ih"
#include "math/box.ih"
#include "common/Ray.ih"
#include "common/Model.ih"
#include "common/safe_gather.ih"
#include "geometry/Geometry.ih"
#include "math/sampling.ih"
#include "transferFunction/TransferFunction.ih"

#include "ospray/OSPDataType.h"

struct DataDrivenSpheres {
  /*! inherit from "Geometry" class: */
  Geometry   super;

  vec3f     *centers; /*! data array that contains the sphere locations */
  float     *data;    /*! data array containing the per-sphere data */

  float     epsilon;

  float     radius0;
  float     radius1;
  float     value0;
  float     value1;

  TransferFunction *uniform transferFunction;          //! Color and opacity transfer function.
};

static void DataDrivenSpheres_postIntersect(uniform Geometry *uniform geometry,
                                  uniform Model *uniform model,
                                  varying DifferentialGeometry &dg,
                                  const varying Ray &ray,
                                  uniform int64 flags)
{
  uniform DataDrivenSpheres *uniform self = (uniform DataDrivenSpheres *uniform)geometry;

  dg.Ng = dg.Ns = ray.Ng;

  float dataval = self->data[ray.primID];
  vec3f color = self->transferFunction->getColorForValue(self->transferFunction, dataval);

  dg.color.x = color.x;
  dg.color.y = color.y;
  dg.color.z = color.z;
  dg.color.w = 1.0;
}

unmasked void DataDrivenSpheres_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  uniform DataDrivenSpheres *uniform self = (uniform DataDrivenSpheres *uniform)args->geometryUserPtr;
  uniform int primID = args->primID;

  uniform vec3f center = self->centers[primID];
  
  uniform float radius;
  if (self->data >= 0 && self->value0 != self->value1)
  {
    uniform float dataval = self->data[primID];
    uniform float d = (dataval - self->value0) / (self->value1 - self->value0);
    if (d > 1) radius = self->radius1;
    else if (d < 0) radius = self->radius0;
    else radius = self->radius0 + d*(self->radius1 - self->radius0);
  }
  else
  {
    radius = self->radius0;
  }

  box3fa *uniform out = (box3fa *uniform)args->bounds_o;
  *out = make_box3fa(center - radius, center + radius);
}

void DataDrivenSpheres_intersect_kernel(const RTCIntersectFunctionNArguments *uniform args,
                              const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex]) return;

  uniform DataDrivenSpheres *uniform self = (uniform DataDrivenSpheres *uniform)args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer toa varying ray!
  varying Ray *uniform ray = (varying Ray *uniform)args->rayhit;

  uniform vec3f center = self->centers[primID];
  
  uniform float radius;
  if (self->data >= 0 && self->value0 != self->value1)
  {
    uniform float dataval = self->data[primID];
    uniform float d = (dataval - self->value0) / (self->value1 - self->value0);
    if (d > 1) radius = self->radius1;
    else if (d < 0) radius = self->radius0;
    else radius = self->radius0 + d*(self->radius1 - self->radius0);
  }
  else
  {
    radius = self->radius0;
  }

  const float approxDist = dot(center - ray->org, ray->dir);
  const vec3f closeOrg = ray->org + approxDist * ray->dir;
  const vec3f A = center - closeOrg;

  const float a = dot(ray->dir,ray->dir);
  const float b = 2.f*dot(ray->dir,A);
  const float c = dot(A,A)-radius*radius;

  const float radical = b*b-4.f*a*c;
  if (radical < 0.f) return;

  const float srad = sqrt(radical);

  const float t_in = (b - srad) *rcpf(2.f*a) + approxDist;
  const float t_out= (b + srad) *rcpf(2.f*a) + approxDist;

  bool hit = false;
  if (t_in > ray->t0 && t_in < ray->t) {
    hit = true;
    ray->t = t_in;
  } else if (t_out > (ray->t0 + self->epsilon) && t_out < ray->t) {
    hit = true;
    ray->t = t_out;
  }

  if (hit) {
    if (isOcclusionTest) {
      ray->t = neg_inf;
    } else {
      ray->primID = primID;
      ray->geomID = self->super.geomID;
      ray->instID = args->context->instID[0];
      // cannot easily be moved to postIntersect
      // we need hit in object space, in postIntersect it is in world-space
      ray->Ng = ray->org + ray->t*ray->dir - center;
    }
  }
}

unmasked void DataDrivenSpheres_intersect(const struct RTCIntersectFunctionNArguments *uniform args)
{
  DataDrivenSpheres_intersect_kernel(args,false);
}

unmasked void DataDrivenSpheres_occluded(const struct RTCIntersectFunctionNArguments *uniform args)
{
  DataDrivenSpheres_intersect_kernel(args,true);
}

export void *uniform DataDrivenSpheres_create(void *uniform cppEquivalent)
{
  uniform DataDrivenSpheres *uniform self = uniform new uniform DataDrivenSpheres;
  Geometry_Constructor(&self->super,cppEquivalent,
                       DataDrivenSpheres_postIntersect,
                       NULL,
                       NULL,
                       0,NULL);
  return self;
}

export void DataDrivenSpheresGeometry_set(void  *uniform _self
                                , void *uniform _model
                                , void *uniform centers
                                , void *uniform data
                                , uniform int numDataDrivenSpheres
                                , uniform float radius0
                                , uniform float radius1
                                , uniform float value0
                                , uniform float value1
                                , void *uniform tf
                                , float *uniform box
                                )
{
  uniform DataDrivenSpheres *uniform self = (uniform DataDrivenSpheres *uniform)_self;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  RTCGeometry geom = rtcNewGeometry(ispc_embreeDevice(),RTC_GEOMETRY_TYPE_USER);
  uniform uint32 geomID = rtcAttachGeometry(model->embreeSceneHandle,geom);

  self->super.model = model;
  self->super.geomID = geomID;
  self->super.numPrimitives = numDataDrivenSpheres;

  self->centers = (vec3f *uniform)centers;
  self->data = (float *uniform)data;

  self->radius0 = radius0;
  self->radius1 = radius1;
  self->value0 = value0;
  self->value1 = value1;

  self->transferFunction = (TransferFunction *uniform)tf;

  // Why a log function?
  self->epsilon = log(self->radius0);
  if (self->epsilon < 0.f)
    self->epsilon = -1.f/self->epsilon;

  // This can occur if the radius is < 1 in some cases, so make sure
  // epsilon is not too big
  if (self->epsilon > self->radius0 / 100.0) {
    self->epsilon = self->radius0 / 100.0;
  }

  uniform int   numDataDrivenSpheres = self->super.numPrimitives;
  vec3f minb, maxb;
  uniform int isFirst = 1;

  foreach (i = 0 ... numDataDrivenSpheres)
  {
    vec3f center = self->centers[i];
    float radius;

    if (self->value0 != self->value1)
    {
      float dataval = self->data[i];
      float d = (dataval - value0) / (value1 - value0);
      if (d > 1) radius = radius1;
      else if (d < 0) radius = radius0;
      else radius = self->radius0 + d*(self->radius1 - self->radius0);
    }
    else
      radius = self->radius0;
  
    if (isFirst)
    {
      isFirst = 0;
      minb = center - radius;
      maxb = center + radius;
    }
    else
    {
      vec3f p = center - radius;
      if (minb.x > p.x) minb.x = p.x;
      if (minb.y > p.y) minb.y = p.y;
      if (minb.z > p.z) minb.z = p.z;

      p = center + radius;
      if (maxb.x < p.x) maxb.x = p.x;
      if (maxb.y < p.y) maxb.y = p.y;
      if (maxb.z < p.z) maxb.z = p.z;
    }
  }
  box[0] = reduce_min(minb.x);
  box[1] = reduce_min(minb.y);
  box[2] = reduce_min(minb.z);
  box[3] = reduce_max(maxb.x);
  box[4] = reduce_max(maxb.y);
  box[5] = reduce_max(maxb.z);

  rtcSetGeometryUserData(geom, self);
  rtcSetGeometryUserPrimitiveCount(geom,numDataDrivenSpheres);
  rtcSetGeometryBoundsFunction
    (geom,(uniform RTCBoundsFunction)&DataDrivenSpheres_bounds, self);
  rtcSetGeometryIntersectFunction
    (geom,(uniform RTCIntersectFunctionN)&DataDrivenSpheres_intersect);
  rtcSetGeometryOccludedFunction
    (geom,(uniform RTCOccludedFunctionN)&DataDrivenSpheres_occluded);
  rtcCommitGeometry(geom);
  rtcReleaseGeometry(geom);
}

