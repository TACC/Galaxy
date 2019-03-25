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
#include "ospray/OSPDataType.h"

struct DataDrivenSpheres {
  /*! inherit from "Geometry" class: */
  Geometry   super;

  /*! data array that contains the sphere data (possibly but not
    necessarily including the color, which could be in color);
    most offset and stride values point into here.  */
  uint8     *data;
  int32      stride;
  int        offset_center;
  int        offset_datavalue;
  int        offset_materialID;
  int        offset_colorID;

  float      radius;
  float      epsilon;
  int        materialID;

  uint8      *color;
  int         color_stride;
  int         color_offset;
  int         color_format; 

  float     radius0;
  float     radius1;
  float     value0;
  float     value1;

  vec2f     *texcoord;

  float     *mappedRadius;

  bool huge_mesh;
};

inline int32 DataDrivenSpheres_getMaterialID(const uniform DataDrivenSpheres *uniform self,
                                   int32 primID)
{
  int32 materialID = -1;
  if (self->offset_materialID >= 0) {
    const uniform int32 primsPerPage = (1024*1024*64);
    if (any(primID >= primsPerPage )) {
      const int primPageID  = primID / primsPerPage;
      const int localPrimID = primID % primsPerPage;
      foreach_unique (primPage in primPageID) {
        uniform uint8 *uniform pagePtr = self->data
          + (((int64)primPage)
             * primsPerPage
             * self->stride);
        uniform uint8 *varying spherePtr = pagePtr
          + self->stride*localPrimID;
        materialID = *((uniform uint32 *varying)(spherePtr+self->offset_materialID));
      }
    } else {
      uniform uint8 *varying spherePtr = self->data
        + self->stride*primID;
      materialID = *((uniform uint32 *varying)(spherePtr+self->offset_materialID));
    }
  } else {
    materialID = self->materialID;
  }
  return materialID;
}

static void DataDrivenSpheres_postIntersect(uniform Geometry *uniform geometry,
                                  uniform Model *uniform model,
                                  varying DifferentialGeometry &dg,
                                  const varying Ray &ray,
                                  uniform int64 flags)
{
  uniform DataDrivenSpheres *uniform self = (uniform DataDrivenSpheres *uniform)geometry;

  dg.Ng = dg.Ns = ray.Ng;

  if ((flags & DG_COLOR) && self->color) {
    uint32 colorID = 0;
    if (self->offset_colorID >= 0) {
      uniform uint8 *varying spherePtr =
        self->data + self->stride*ray.primID;
      colorID = *((uniform uint32 *varying)(spherePtr+self->offset_colorID));
    } else
      colorID = ray.primID;
    if (self->color_format == OSP_FLOAT4) {
      dg.color = gather_byte_stride_vec4f(self->huge_mesh,
                                          self->color + self->color_offset,
                                          self->color_stride,
                                          colorID);
    } else if (self->color_format == OSP_FLOAT3
        || self->color_format == OSP_FLOAT3A)
    {
      vec3f c = gather_byte_stride_vec3f(self->huge_mesh,
                                         self->color + self->color_offset,
                                         self->color_stride,
                                         colorID);
      dg.color = make_vec4f(c.x, c.y, c.z, 1.f);
    } else if (self->color_format == OSP_UCHAR4) {
      const uint32 c = gather_stride_uint32(self->huge_mesh,
                                            self->color + self->color_offset,
                                            self->color_stride,
                                            colorID);
      dg.color.x = (c & 0x000000ff) / 255.0;
      dg.color.y = ((c & 0x0000ff00) >> 8) / 255.0;
      dg.color.z = ((c & 0x00ff0000) >> 16) / 255.0;
      dg.color.w = ((c & 0xff000000) >> 24) / 255.0;
    }
  }

  if (flags & DG_TEXCOORD && self->texcoord)
    dg.st = self->texcoord[ray.primID];
  else
    dg.st = make_vec2f(0.0f);

  if (flags & DG_MATERIALID) {
    dg.materialID = DataDrivenSpheres_getMaterialID(self, ray.primID);
  }
}

unmasked void DataDrivenSpheres_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  uniform DataDrivenSpheres *uniform self = (uniform DataDrivenSpheres *uniform)args->geometryUserPtr;
  uniform int primID = args->primID;
  uniform uint8 *uniform spherePtr = self->data + self->stride*((uniform int64)primID);
  uniform float radius = self->mappedRadius ? self->mappedRadius[primID] : self->radius;
  uniform vec3f center = *((uniform vec3f*uniform)(spherePtr+self->offset_center));
  box3fa *uniform out = (box3fa *uniform)args->bounds_o;
  *out = make_box3fa(center - radius, center + radius);
}

void DataDrivenSpheres_intersect_kernel(const RTCIntersectFunctionNArguments *uniform args,
                              const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex]) return;

  uniform DataDrivenSpheres *uniform self
    = (uniform DataDrivenSpheres *uniform)args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer toa varying ray!
  varying Ray *uniform ray = (varying Ray *uniform)args->rayhit;

  // from here on it's the original intersection code from ospray/embree2
  uniform uint8 *uniform spherePtr =
    self->data + self->stride*((uniform int64)primID);
  uniform float radius = self->mappedRadius ? self->mappedRadius[primID] : self->radius;
  uniform vec3f center = *((uniform vec3f*uniform)(spherePtr+self->offset_center));
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

int32 DataDrivenSpheres_getMaterialID(
                            const Geometry *uniform const _self
                            , const int32 primID
                            )
{
  const DataDrivenSpheres *const uniform self = (const DataDrivenSpheres *uniform)_self;
  return DataDrivenSpheres_getMaterialID(self, primID);
}

SampleAreaRes DataDrivenSpheres_sampleArea(
                                 const Geometry *uniform const _self
                                 , const int32 primID
                                 , const uniform affine3f &xfm
                                 , const uniform affine3f &rcp_xfm
                                 , const vec2f& s
                                 )
{
  const DataDrivenSpheres *const uniform self = (const DataDrivenSpheres *uniform)_self;
  float radius = (self->mappedRadius) ? self->mappedRadius[primID] : self->radius;
  vec3f center;

  const uniform int32 primsPerPage = (1024*1024*64);
  if (any(primID >= primsPerPage )) {
    const int primPageID  = primID / primsPerPage;
    const int localPrimID = primID % primsPerPage;
    foreach_unique(primPage in primPageID) {
      uint8 *uniform pagePtr = self->data
        + (((int64)primPage)
           * primsPerPage
           * self->stride);
      uint8 *spherePtr = pagePtr + self->stride*localPrimID;
      center = *((vec3f*)(spherePtr+self->offset_center));
      if (self->mappedRadius >= 0) radius = self->mappedRadius[localPrimID];
    }
  } else {
    uint8 *spherePtr = self->data + self->stride*primID;
    center = *((vec3f*)(spherePtr+self->offset_center));
  }

  const vec3f localNormal = uniformSampleSphere(radius, s);
  const vec3f localPos = center + localNormal;

  SampleAreaRes res;
  res.pos = xfmPoint(xfm, localPos);
  res.normal = normalize(xfmVector(transposed(rcp_xfm.l), localNormal));

  return res;
}

void DataDrivenSpheres_getAreas(
                      const Geometry *const uniform _self
                      , const int32 *const uniform primIDs
                      , const uniform int32 numPrims
                      , const uniform affine3f &xfm
                      , float *const uniform areas
                      )
{
  DataDrivenSpheres *uniform self = (DataDrivenSpheres *uniform)_self;

  // detect (non-uniform) scaling; get length of transformed unit-vectors
  const uniform vec3f scaling3 =
    make_vec3f(length(xfm.l.vx), length(xfm.l.vy), length(xfm.l.vz));
  const uniform float min_scaling = reduce_min(scaling3);
  const uniform float max_scaling = reduce_max(scaling3);
  if ((max_scaling - min_scaling) > 1e-4f*min_scaling)
    print("#osp:DataDrivenSpheres_getAreas: Non-uniform scaling in instance "
          "transformation detected! Importance sampling for emissive "
          "materials and thus resulting image may be wrong.\n");
  // use average as approximation to closest uniform scaling
  const uniform float scaling = reduce_add(scaling3)/3.f;

  if (self->offset_datavalue < 0) {
    const uniform float sphereArea = four_pi * sqr(self->radius * scaling);
    foreach (i = 0 ... numPrims)
      areas[i] = sphereArea;
  } else {
    const uniform int64 stride = self->stride;
    // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
    for (uniform int32 i = 0; i < numPrims; i++) {
      const uniform float radius = self->mappedRadius[primIDs[i]];
      areas[i] = four_pi * sqr(radius * scaling);
    }
  }
}


export void *uniform DataDrivenSpheres_create(void *uniform cppEquivalent)
{
  uniform DataDrivenSpheres *uniform self = uniform new uniform DataDrivenSpheres;
  Geometry_Constructor(&self->super,cppEquivalent,
                       DataDrivenSpheres_postIntersect,
                       NULL,
                       NULL,
                       0,NULL);

  self->mappedRadius = NULL;

  return self;
}

export void *uniform DataDrivenSpheres_freeMappedRadius(void  *uniform _self)
{
  uniform DataDrivenSpheres *uniform self = (uniform DataDrivenSpheres *uniform)_self;
  if (self->mappedRadius)
  {
    print("freeing mapped radius data\n");
    delete[] self->mappedRadius;
    self->mappedRadius = NULL;
  }
}

export void DataDrivenSpheresGeometry_set(void  *uniform _self
                                , void *uniform _model
                                , void *uniform data
                                , void *uniform materialList
                                , vec2f *uniform texcoord
                                , void *uniform color
                                , uniform int color_offset
                                , uniform int color_stride
                                , uniform int color_format
                                , uniform int numDataDrivenSpheres
                                , uniform int bytesPerSphere
                                , uniform float radius
                                , uniform int materialID
                                , uniform int offset_center
                                , uniform int offset_datavalue
                                , uniform int offset_materialID
                                , uniform int offset_colorID
                                , uniform bool huge_mesh
                                , uniform float radius0
                                , uniform float radius1
                                , uniform float value0
                                , uniform float value1
                                )
{
  uniform DataDrivenSpheres *uniform self = (uniform DataDrivenSpheres *uniform)_self;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  RTCGeometry geom = rtcNewGeometry(ispc_embreeDevice(),RTC_GEOMETRY_TYPE_USER);
  uniform uint32 geomID = rtcAttachGeometry(model->embreeSceneHandle,geom);

  self->super.model = model;
  self->super.geomID = geomID;
  self->super.numPrimitives = numDataDrivenSpheres;
  self->super.getAreas = DataDrivenSpheres_getAreas;
  self->super.sampleArea = DataDrivenSpheres_sampleArea;
  self->super.materialList = (Material **)materialList;
  self->texcoord = texcoord;
  self->color = (uint8 *uniform)color;
  self->color_stride = color_stride;
  self->color_offset = color_offset;
  self->color_format = color_format;
  self->radius = radius;
  self->data = (uint8 *uniform)data;
  self->materialID = materialID;
  self->stride = bytesPerSphere;

  self->offset_center     = offset_center;
  self->offset_datavalue  = offset_datavalue;
  self->offset_materialID = offset_materialID;
  self->offset_colorID    = offset_colorID;

  self->radius0 = radius0;
  self->radius1 = radius1;
  self->value0 = value0;
  self->value1 = value1;

  self->huge_mesh = huge_mesh;

  // Why a log function?
  self->epsilon = log(self->radius);
  if (self->epsilon < 0.f)
    self->epsilon = -1.f/self->epsilon;

  // This can occur if the radius is < 1 in some cases, so make sure
  // epsilon is not too big
  if (self->epsilon > self->radius / 100.0) {
    self->epsilon = self->radius / 100.0;
  }

  if (self->mappedRadius)
  {
    delete[] self->mappedRadius;
    self->mappedRadius = NULL;
  }

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

export void DataDrivenSpheresGeometry_computeRadius(void  *uniform _self, void *uniform _box)
{
  uniform DataDrivenSpheres *uniform self = (uniform DataDrivenSpheres *uniform)_self;
  uniform box3f *uniform box = (uniform box3f *uniform)_box;

  uniform float radius0 = self->radius0;
  uniform float radius1 = self->radius1;
  uniform float value0 = self->value0;
  uniform float value1 = self->value1;

  uniform int   numDataDrivenSpheres = self->super.numPrimitives;

  if (self->mappedRadius)
  {
    delete[] self->mappedRadius;
    self->mappedRadius = NULL;
  }

  if (self->offset_datavalue >= 0 && self->radius0 >= 0.0)
  {
    self->mappedRadius = uniform new float[numDataDrivenSpheres];

    uniform uint8 *base = self->data + self->offset_datavalue;
    foreach (i = 0 ... numDataDrivenSpheres)
    {
      varying float dataval = *((uniform float *varying)(base + i*self->stride));
      float d = (dataval - value0) / (value1 - value0);
      if (d > 1) self->mappedRadius[i] = radius1;
      else if (d < 0) self->mappedRadius[i] = radius0;
      else self->mappedRadius[i] = radius0 + d*(radius1 - radius0);
    }
  }

  uniform uint8 *uniform base = (uniform uint8 *uniform)(self->data + self->offset_center);

  vec3f *p = (vec3f *)base;
  
  float minx = p->x;
  float miny = p->y;
  float minz = p->z;

  float maxx = p->x;
  float maxy = p->y;
  float maxz = p->z;

  foreach (i = 0 ... numDataDrivenSpheres)
  {
    float radius = self->mappedRadius ? self->mappedRadius[i] : self->radius;
    vec3f *ptr = (vec3f *)(base + i * self->stride);
    vec3f center = *ptr;

    vec3f p = center - radius;
    if (minx > p.x) minx = p.x;
    if (miny > p.y) miny = p.y;
    if (minz > p.z) minz = p.z;

    p = center + radius;
    if (maxx < p.x) maxx = p.x;
    if (maxy < p.y) maxy = p.y;
    if (maxz < p.z) maxz = p.z;
  }

  box->lower.x = reduce_min(minx);
  box->lower.y = reduce_min(miny);
  box->lower.z = reduce_min(minz);

  box->upper.x = reduce_max(maxx);
  box->upper.y = reduce_max(maxy);
  box->upper.z = reduce_max(maxz);
}
