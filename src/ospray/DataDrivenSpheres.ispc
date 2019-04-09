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

  int deadbeef;

  /*! data array that contains the sphere data (possibly but not
    necessarily including the color, which could be in color);
    most offset and stride values point into here.  */
  uint8     *data;
  int32      stride;
  int        offset_center;
  int        offset_datavalue;
  int        offset_materialID;
  int        offset_colorID;

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


  TransferFunction *uniform transferFunction;          //! Color and opacity transfer function.
  vec2f     *texcoord;

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

  uniform uint8 *base = self->data + self->offset_datavalue;
  varying float dataval = *((uniform float *varying)(base + ray.primID*self->stride));

  vec3f color = self->transferFunction->getColorForValue(self->transferFunction, dataval);
  dg.color.x = color.x;
  dg.color.y = color.y;
  dg.color.z = color.z;
  dg.color.w = 1.0;

// print("D %\nR %\nG %\nB %\n", dataval, dg.color.x, dg.color.y, dg.color.z);

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
  uniform float radius;

// print("B\n");

  if (self->offset_datavalue >= 0 && self->value0 != self->value1)
  {
    uniform float dataval = *((uniform float *uniform)(spherePtr + self->offset_datavalue));

    uniform float d = (dataval - self->value0) / (self->value1 - self->value0);
    if (d > 1) radius = self->radius1;
    else if (d < 0) radius = self->radius0;
    else radius = self->radius0 + d*(self->radius1 - self->radius0);
// print ("B1 % % %\n", dataval, d, radius);
  }
  else
  {
    radius = self->radius0;
// print ("B2 %\n", radius);
  }

  uniform vec3f center = *((uniform vec3f*uniform)(spherePtr+self->offset_center));
// print("CX %\nCY %\nCZ %\n", center.x, center.y, center.z);

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

  uniform uint8 *uniform spherePtr = self->data + self->stride*((uniform int64)primID);
  uniform float radius;

  if (self->offset_datavalue >= 0 && self->value0 != self->value1)
  {
    uniform float dataval = *((uniform float *)(spherePtr + self->offset_datavalue));
    uniform float d = (dataval - self->value0) / (self->value1 - self->value0);
    if (d > 1) radius = self->radius1;
    else if (d < 0) radius = self->radius0;
    else radius = self->radius0 + d*(self->radius1 - self->radius0);
  }
  else
    radius = self->radius0;

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

  float radius;
  uint8 *spherePtr;
  vec3f center;

  const uniform int32 primsPerPage = (1024*1024*64);
  if (any(primID >= primsPerPage ))
  {
    const int primPageID  = primID / primsPerPage;
    const int localPrimID = primID % primsPerPage;

    foreach_unique(primPage in primPageID) {
      uint8 *uniform pagePtr = self->data + (((int64)primPage) * primsPerPage * self->stride);
      spherePtr = pagePtr + self->stride*localPrimID;
    }
  }
  else
  {
    spherePtr = self->data + self->stride*primID;
  }

  center = *((vec3f*)(spherePtr+self->offset_center));
  if (self->offset_datavalue >= 0)
  {
    float dataval = *((float *)(spherePtr + self->offset_datavalue));
    float d = (dataval - self->value0) / (self->value1 - self->value0);
    if (d > 1) radius = self->radius1;
    else if (d < 0) radius = self->radius0;
    else radius = self->radius0 + d*(self->radius1 - self->radius0);
  }
  else
    radius = self->radius0;

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

  if (self->offset_datavalue < 0)
  {
    const uniform float sphereArea = four_pi * sqr(self->radius0 * scaling);
    foreach (i = 0 ... numPrims)
      areas[i] = sphereArea;
  }
  else
  {
    const uniform int64 stride = self->stride;
    uniform float radius;

    // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
    for (uniform int32 i = 0; i < numPrims; i++)
    {
      if (self->offset_datavalue >= 0)
      {
        uniform uint8 *uniform spherePtr = self->data + self->stride*primIDs[i];
        uniform float dataval = *((uniform float *uniform )(spherePtr + self->offset_datavalue));
        uniform float d = (dataval - self->value0) / (self->value1 - self->value0);
        if (d > 1) radius = self->radius1;
        else if (d < 0) radius = self->radius0;
        else radius = self->radius0 + d*(self->radius1 - self->radius0);
      }
      else
        radius = self->radius0;
        
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
  return self;
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
  self->super.getAreas = DataDrivenSpheres_getAreas;
  self->super.sampleArea = DataDrivenSpheres_sampleArea;
  self->super.materialList = (Material **)materialList;
  self->texcoord = texcoord;
  self->color = (uint8 *uniform)color;
  self->color_stride = color_stride;
  self->color_offset = color_offset;
  self->color_format = color_format;
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

  self->transferFunction = (TransferFunction *uniform)tf;

  self->huge_mesh = huge_mesh;

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

  if (self->offset_datavalue >= 0 && (self->value0 != self->value1))
  {
    foreach (i = 0 ... numDataDrivenSpheres)
    {
      uint8 *spherePtr = self->data + self->stride * i;
      vec3f center = *((vec3f*)(spherePtr + self->offset_center));
      float dataval = *((float*)(spherePtr + self->offset_datavalue));

      float d = (dataval - value0) / (value1 - value0);

      float radius;

      if (d > 1) radius = radius1;
      else if (d < 0) radius = radius0;
      else radius = self->radius0 + d*(self->radius1 - self->radius0);
  
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
  }
  else
  {
    uniform uint8 *base = self->data + self->offset_datavalue;
    uniform float radius = self->radius0;

    foreach (i = 0 ... numDataDrivenSpheres)
    {
      uint8 *spherePtr = self->data + self->stride * i;
      vec3f center = *((vec3f*)(spherePtr + self->offset_center));
    
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

