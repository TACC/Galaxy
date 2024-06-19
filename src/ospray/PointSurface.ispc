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

struct PointSurface {
  /*! inherit from "Geometry" class: */
  Geometry   super;

  vec3f     *points;  /*! data array that contains the points locations */
};

static void PointSurface_postIntersect(uniform Geometry *uniform geometry,
                                  uniform Model *uniform model,
                                  varying DifferentialGeometry &dg,
                                  const varying Ray &ray,
                                  uniform int64 flags)
{
#if 0
  uniform PointSurface *uniform self = (uniform PointSurface *uniform)geometry;

  dg.Ng = dg.Ns = ray.Ng;

  float dataval = self->data[ray.primID];
  vec3f color = self->transferFunction->getColorForValue(self->transferFunction, dataval);

  dg.color.x = color.x;
  dg.color.y = color.y;
  dg.color.z = color.z;
  dg.color.w = 1.0;
#endif
}

unmasked void PointSurface_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  uniform PointSurface *uniform self = (uniform PointSurface *uniform)args->geometryUserPtr;
  uniform int primID = args->primID;

// AAAAAAAAAAAAAAAAA This needs to know R!
  uniform vec3f point = self->points[primID];

  box3fa *uniform out = (box3fa *uniform)args->bounds_o;
  *out = make_box3fa(point, point);
}

void PointSurface_intersect_kernel(const RTCIntersectFunctionNArguments *uniform args,
                              const uniform bool isOcclusionTest)
{
#if 0
  // make sure to set the mask
  if (!args->valid[programIndex]) return;

  uniform PointSurface *uniform self = (uniform PointSurface *uniform)args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer toa varying ray!
  varying Ray *uniform ray = (varying Ray *uniform)args->rayhit;

  uniform vec3f point = self->points[primID];
  
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

  const float approxDist = dot(point - ray->org, ray->dir);
  const vec3f closeOrg = ray->org + approxDist * ray->dir;
  const vec3f A = point - closeOrg;

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
      ray->Ng = ray->org + ray->t*ray->dir - point;
    }
  }

#endif
}

unmasked void PointSurface_intersect(const struct RTCIntersectFunctionNArguments *uniform args)
{
  PointSurface_intersect_kernel(args,false);
}

unmasked void PointSurface_occluded(const struct RTCIntersectFunctionNArguments *uniform args)
{
  PointSurface_intersect_kernel(args,true);
}

export void *uniform PointSurface_create(void *uniform cppEquivalent)
{
  uniform PointSurface *uniform self = uniform new uniform PointSurface;
  Geometry_Constructor(&self->super,cppEquivalent,
                       PointSurface_postIntersect,
                       NULL,
                       NULL,
                       0,NULL);
  return self;
}

export void PointSurfaceGeometry_set(void  *uniform _self
                                , void *uniform _model
                                , void *uniform points
                                , uniform int numPoints
                                , float *uniform box
                                )
{
  uniform PointSurface *uniform self = (uniform PointSurface *uniform)_self;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  RTCGeometry geom = rtcNewGeometry(ispc_embreeDevice(),RTC_GEOMETRY_TYPE_USER);
  uniform uint32 geomID = rtcAttachGeometry(model->embreeSceneHandle,geom);

  self->super.model = model;
  self->super.geomID = geomID;
  self->super.numPrimitives = numPoints;

  self->points = (vec3f *uniform)points;

  uniform int numPoints = self->super.numPrimitives;
  vec3f minb, maxb;
  uniform int isFirst = 1;

  foreach (i = 0 ... numPoints)
  {
    vec3f p = self->points[i];
    float radius;

    if (isFirst)
    {
      isFirst = 0;
      minb = p;
      maxb = p;
    }
    else
    {
      if (minb.x > p.x) minb.x = p.x;
      if (minb.y > p.y) minb.y = p.y;
      if (minb.z > p.z) minb.z = p.z;

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
  rtcSetGeometryUserPrimitiveCount(geom,numPoints);
  rtcSetGeometryBoundsFunction
    (geom,(uniform RTCBoundsFunction)&PointSurface_bounds, self);
  rtcSetGeometryIntersectFunction
    (geom,(uniform RTCIntersectFunctionN)&PointSurface_intersect);
  rtcSetGeometryOccludedFunction
    (geom,(uniform RTCOccludedFunctionN)&PointSurface_occluded);
  rtcCommitGeometry(geom);
  rtcReleaseGeometry(geom);
}

