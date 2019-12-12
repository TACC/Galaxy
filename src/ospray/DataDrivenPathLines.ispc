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
#include "geometry/Geometry.ih"
#include "math/sampling.ih"

struct DataDrivenPathLines
{
  uniform Geometry super; //!< inherited geometry fields

  const uniform vec4f  *vertices;
  const uniform uint32 *indices;
  int32           numVertices;

  uniform float  radius0, radius1, value0, value1;
  TransferFunction *uniform transferFunction;          //! Color and opacity transfer function.
};

unmasked void DataDrivenPathLines_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  uniform DataDrivenPathLines *uniform self = (uniform DataDrivenPathLines *uniform)args->geometryUserPtr;
  uniform int primID = args->primID;
  const uniform uint32 index  = self->indices[primID];

  const uniform vec3f A = make_vec3f(self->vertices[index]);
  const uniform vec3f B = make_vec3f(self->vertices[index+1]);

  float dA, dB;
  float rA, rB;

  if (self->value0 != self->value1)
  {
    float d;

    dA = ((float *)(&self->vertices[index]))[3];
    d = (d - self->value0) / (self->value1 - self->value1);
    rA = (dA < 0) ? self->radius0 : (d > 1.0) ? self->radius1 : (self->radius0 + d*(self->radius1 - self->radius0));

    dB = ((float *)(&self->vertices[index+1]))[3];
    d = (dB - self->value0) / (self->value1 - self->value1);
    rB = (d < 0) ? self->radius0 : (d > 1.0) ? self->radius1 : (self->radius0 + d*(self->radius1 - self->radius0));
  }
  else
  {
    rA = rB = self->radius0;
  }

  box3fa *uniform out = (box3fa *uniform)args->bounds_o;
  *out = make_box3fa(min(A,B) - self->radius0,
                     max(A,B) + self->radius0);
}

/*! do a ray-sphere intersection, with the numerical fix of first
    moving the ray origin to an approximate sphere distance, and then
    re-adding this approximate distance to the computed hit
    distance */
inline void intersectSphere_numericalFix(varying Ray &ray,
                                         /*! center/positoin of the sphere  */
                                         const vec3f &center,
                                         float &t_in, float &t_out,
                                         const uniform float r)
{
  const float approximateDistance = dot(center-ray.org,ray.dir);

  const vec3f closeOrg = ray.org + approximateDistance * ray.dir;
  const vec3f A = center - closeOrg;
  const float a = dot(ray.dir,ray.dir);
  const float b = -2.f*dot(ray.dir,A);
  const float c = dot(A,A)-r*r;

  const float radical = b*b-4.f*a*c;
  if (radical < 0.f)
    return;

  const float srad = sqrt(radical);

  const float sph_t_in  = approximateDistance + (- b - srad) * rcpf(2.f*a);
  const float sph_t_out = approximateDistance + (- b + srad) * rcpf(2.f*a);
  t_in = min(t_in, sph_t_in);
  t_out= max(t_out,sph_t_out);
}

/*! do a ray-cylinder intersection, with the numerical fix of first
    moving the ray origin to an approximate distance, and then
    re-adding this approximate distance to the computed hit
    distance. for the approximate distance we use the distance from
    ray origin to center of the cylinder (projected to the ray) */
inline void intersectCylinder_numericalFix(varying Ray &ray,
                                           const vec3f &_A, const vec3f &_B,
                                           float &t_in, float &t_out,
                                           const uniform float r)
{
  const vec3f center = 0.5f*(_A+_B);
  const float approximateDistance = dot(center-ray.org,ray.dir);

  const vec3f closeOrg = ray.org + approximateDistance * ray.dir;
  const vec3f A = _A - closeOrg;
  const vec3f B = _B - closeOrg;

  const vec3f V = ray.dir;

  const vec3f AB = B - A;

  const vec3f AOxAB = cross(AB,A);
  const vec3f VxAB  = cross(V,AB);
  const float ab2   = dot(AB,AB);
  const float a     = dot(VxAB,VxAB);
  if (a == 0.f) return;
  const float b     = 2 * dot(VxAB,AOxAB);
  const float c     = dot(AOxAB,AOxAB) - (sqr(r) * ab2);

  // ------------------------------------------------------------------
  // abc formula: t0,1 = (-b +- sqrt(b^2-4*a*c)) / 2a
  //
  const float radical = b*b-4.f*a*c;
  if (radical < 0.f)
    return;

  const float srad = sqrt(radical);

  // clip to near and far cap of cylinder
  const float dVAB = dot(V,AB);
  if (dVAB == 0.f) return;
  const float rVAB = rcp(dVAB);
  const float tA = dot(AB,A) * rVAB;
  const float tB = dot(AB,B) * rVAB;

  const float cyl_t_in = approximateDistance + max((- b - srad)*rcpf(2.f*a), min(tA,tB));
  const float cyl_t_out= approximateDistance + min((- b + srad)*rcpf(2.f*a), max(tA,tB));

  if (cyl_t_in < cyl_t_out) {
    t_in = cyl_t_in;
    t_out = cyl_t_out;
  }
}

unmasked void DataDrivenPathLines_intersect_kernel(
  const struct RTCIntersectFunctionNArguments *uniform args,
  const uniform bool occlusionTest
)
{
  // make sure to set the mask
  if (!args->valid[programIndex]) return;

  uniform DataDrivenPathLines *uniform self
    = (uniform DataDrivenPathLines *uniform)args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer toa varying ray!
  varying Ray *uniform ray = (varying Ray *uniform)args->rayhit;

  const uniform uint32 idx = self->indices[primID];
  const vec3f A = make_vec3f(self->vertices[idx]);
  const vec3f B = make_vec3f(self->vertices[idx+1]);

  if (self->value0 != self->value1)
  {
    float d0 = ((float *)(&self->vertices[idx]))[3];
    float d1 = ((float *)(&self->vertices[idx+1]))[3];
  }

  float t_in=inf, t_out=-inf;
  intersectCylinder_numericalFix(*ray,A,B,t_in,t_out,self->radius0);
  intersectSphere_numericalFix(*ray,A,t_in,t_out,self->radius0);
  intersectSphere_numericalFix(*ray,B,t_in,t_out,self->radius0);

  bool hit = false;
  if (t_in < t_out) {
    if (t_in > ray->t0 && t_in < ray->t) {
      ray->t = t_in;
      hit = true;
    } else if (t_out > ray->t0 && t_out < ray->t) {
      ray->t = t_out;
      hit = true;
    }
  }

  if (hit) {
    if (occlusionTest) {
      ray->t = neg_inf;
    } else {
      ray->geomID = self->super.geomID;
      ray->primID = primID;
      ray->instID = args->context->instID[0];
      const vec3f P = ray->org + ray->t * ray->dir;
      float s = dot(P-A,B-A) * rcpf(dot(B-A,B-A));
      s = min(max(s,0.f),1.f);
      const vec3f PonAxis = A + s * (B-A);
      ray->u = s;
      ray->Ng = normalize(P-PonAxis);
    }
  }
}

unmasked void DataDrivenPathLines_intersect(const struct RTCIntersectFunctionNArguments *uniform args)
{
  DataDrivenPathLines_intersect_kernel(args,false);
}

unmasked void DataDrivenPathLines_occluded(const struct RTCIntersectFunctionNArguments *uniform args)
{
  DataDrivenPathLines_intersect_kernel(args,true);
}

static void DataDrivenPathLines_postIntersect(uniform Geometry *uniform self,
                                       uniform Model *uniform model,
                                       varying DifferentialGeometry &dg,
                                       const varying Ray &ray,
                                       uniform int64 flags)
{
  dg.Ng = dg.Ns = ray.Ng;

  if ((flags & DG_COLOR)) {
    uniform DataDrivenPathLines *uniform self = (uniform DataDrivenPathLines *uniform)self;

    if (self->transferFunction)
    {
      vec4f *v4 = (vec4f *)self->vertices;

      const varying uint32 index  = self->indices[ray.primID];

      float Z = ((1.f-ray.u) * v4[index].z) + (ray.u * v4[index+1].z);
      float radius = ((1.f-ray.u) * self->vertices[index].w) + (ray.u * self->vertices[index+1].w);

      float dataval;
      if (self->value0 == self->value1) dataval = self->radius0;
      else if (self->radius0 < self->radius1 && radius < self->radius0)      dataval = self->value0; 
      else if (self->radius0 < self->radius1 && radius > self->radius1) dataval = self->value1; 
      else if (self->radius0 > self->radius1 && radius < self->radius1) dataval = self->value1; 
      else if (self->radius0 > self->radius1 && radius > self->radius0) dataval = self->value0; 
      else dataval = self->value0 + ((radius - self->radius0) / (self->radius1 - self->radius0))*(self->value1 - self->value0); 

      vec3f color = self->transferFunction->getColorForValue(self->transferFunction, dataval);

      dg.color.x = color.x;
      dg.color.y = color.y;
      dg.color.z = color.z;
      dg.color.w = 1.0;
    }
    else
    {
      dg.color.x = 1.0;
      dg.color.y = 1.0;
      dg.color.z = 1.0;
      dg.color.w = 1.0;
    }
  }
}

export void *uniform DataDrivenPathLines_create(void *uniform cppEquivalent)
{
  DataDrivenPathLines *uniform geom = uniform new DataDrivenPathLines;
  Geometry_Constructor(&geom->super,cppEquivalent,
                       DataDrivenPathLines_postIntersect,
                       NULL,
                       NULL,
                       0,NULL);
  return geom;
}


export void *uniform
DataDrivenPathLines_setCurve(void           *uniform _self,
                           void           *uniform _model,
                     const uniform float  *uniform vertices,
                           int32           uniform numVertices,
                     const uniform uint32 *uniform indices,
                           int32           uniform numSegments,
                     void *uniform tf,
                     const uniform float  r0,
                     const uniform float  r1,
                     const uniform float  v0,
                     const uniform float  v1)
{
  DataDrivenPathLines *uniform self = (DataDrivenPathLines *uniform)_self;

  Model *uniform model = (Model *uniform)_model;

  self->transferFunction = (TransferFunction *uniform)tf;

  self->vertices = (vec4f *)vertices;
  self->indices  = indices;

  self->radius0 = r0;
  self->radius1 = r1;
  self->value0 = v0;
  self->value1 = v1;


  uniform RTCGeometry geom = rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_ROUND_BEZIER_CURVE);
  rtcSetSharedGeometryBuffer(geom,RTC_BUFFER_TYPE_VERTEX,0,RTC_FORMAT_FLOAT4, vertices,0,sizeof(uniform vec3fa),numVertices);
  rtcSetSharedGeometryBuffer(geom,RTC_BUFFER_TYPE_INDEX,0,RTC_FORMAT_UINT, indices,0,sizeof(uniform int),numSegments);
  uniform uint32 geomID = rtcAttachGeometry(model->embreeSceneHandle,geom);
  rtcCommitGeometry(geom);
  rtcReleaseGeometry(geom);

  self->super.geom = geom;

  self->super.model = model;
  self->super.geomID = geomID;
  self->super.numPrimitives = numSegments;
  self->super.postIntersect = DataDrivenPathLines_postIntersect;

  // TODO implement area sampling of curves for geometry lights
  self->numVertices = 0; // not used by curve
}
