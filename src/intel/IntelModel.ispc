#include "embree3/rtcore_config.h"

export void
crap(uniform int *uniform foo) {}



#include "embree3/rtcore_ray.isph"
#include "embree3/rtcore.isph"

#include "IntelModel.ih"
#include "EmbreeGeometry.ih"
#include "Rays.ih"
#include "VolumeVis.ih"
#include "VklVolume.ih"

#if TARGET_WIDTH == 1
#define INTERSECTOR rtcIntersect
#elif TARGET_WIDTH == 4
#define INTERSECTOR rtcIntersect4
#elif TARGET_WIDTH == 8
#define INTERSECTOR rtcIntersect8
#elif TARGET_WIDTH == 16
#define INTERSECTOR rtcIntersect16
#else
#error Unknown vector width
#endif


void
IntelModel_IntersectSOA(RTCScene scene, void *uniform rayhit)
{
  uniform RTCIntersectContext context;
  rtcInitIntersectContext(&context);

  varying int valid = -1;
  INTERSECTOR((const int* uniform)&valid, scene, &context, rayhit);
}

struct rayOD
{
  vec3f o;
  vec3f d;
};

export void
IntelModel_TraceRays(uniform IntelModel_ispc& self, const uniform int n, void *uniform _rays)
{
  RTCScene scene = self.scene;
  EmbreeGeometry_ispc **uniform geom = self.geometries;
  uniform RayList_ispc *uniform rays = (uniform RayList_ispc *)_rays;
  uniform nV = self.nVolumes;
  uniform VolumeVis_ispc **uniform volume_vis = (uniform VolumeVis_ispc **)self.volume_vis;
  uniform VklVolume_ispc **uniform volumes = (uniform VklVolume_ispc **)self.volumes;

  RTCRayHit rayhit;

  uniform float step = -1;
  uniform float epsilon;

  for (uniform int major = 0; major < nV; major++)
  {
    uniform VolumeVis_ispc *uniform vvis = volume_vis[major];
    uniform VklVolume_ispc *uniform vol = (uniform VklVolume_ispc *uniform)volumes[major];

#if 0
    if (vvis->volume_render)
      integrate = true;

    if (vvis->nIsovalues > 0)
      integrate = true;

    uniform float s = vol->samplingStep * vol->samplingRate;

    if (step < 0 || step > s)
    {
      step = s;
      epsilon = global_epsilon * s;
    }
#endif
  }

  foreach (i = 0 ... n)
  {
    uniform RTCIntersectContext context;
    rtcInitIntersectContext(&context);

    rayhit.ray.org_x     = rays->ox[i];
    rayhit.ray.org_y     = rays->oy[i];
    rayhit.ray.org_z     = rays->oz[i];
    rayhit.ray.dir_x     = rays->dx[i];
    rayhit.ray.dir_y     = rays->dy[i];
    rayhit.ray.dir_z     = rays->dz[i];
    rayhit.ray.tnear     = rays->t[i];
    rayhit.ray.tfar      = rays->tMax[i];
    rayhit.ray.mask      = -1;
    rayhit.ray.flags     = 0;
    rayhit.hit.geomID    = RTC_INVALID_GEOMETRY_ID;
    rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    varying rayOD *uniform r = (varying rayOD *uniform)&rayhit;

    IntelModel_IntersectSOA(scene, (void *)&rayhit);
}
}

// This is for testing the Embree device interface from C++.   It is equivalent to
// the TraceRays_TraceRays in renderer/TraceRays.ispc - a call made from C++ that
// bridges the gap from C++ to ISPC.

export void
IntelModel_Intersect(uniform IntelModel_ispc& self, const uniform int n, void *uniform _rays)
{
  RTCScene scene = self.scene;
  EmbreeGeometry_ispc **uniform geom = self.geometries;
  uniform RayList_ispc *uniform rays = (uniform RayList_ispc *)_rays;
  RTCRayHit rayhit;

  foreach (i = 0 ... n)
  {
    uniform RTCIntersectContext context;
    rtcInitIntersectContext(&context);

    rayhit.ray.org_x     = rays->ox[i];
    rayhit.ray.org_y     = rays->oy[i];
    rayhit.ray.org_z     = rays->oz[i];
    rayhit.ray.dir_x     = rays->dx[i];
    rayhit.ray.dir_y     = rays->dy[i];
    rayhit.ray.dir_z     = rays->dz[i];
    rayhit.ray.tnear     = rays->t[i];
    rayhit.ray.tfar      = rays->tMax[i];
    rayhit.ray.mask      = -1;
    rayhit.ray.flags     = 0;
    rayhit.hit.geomID    = RTC_INVALID_GEOMETRY_ID;
    rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    varying rayOD *uniform r = (varying rayOD *uniform)&rayhit;

    IntelModel_IntersectSOA(scene, (void *)&rayhit);

    int gid = rayhit.hit.geomID;
    if (gid != RTC_INVALID_GEOMETRY_ID && geom[gid]->connectivity && geom[gid]->normals)
    {
        vec3f bary = make_vec3f(1.0f - rayhit.hit.u - rayhit.hit.v, rayhit.hit.u, rayhit.hit.v);
        vec3i pqr = ((vec3i*)geom[gid]->connectivity)[rayhit.hit.primID];
        vec3f np = ((vec3f*)geom[gid]->normals)[pqr.x];
        vec3f nq = ((vec3f*)geom[gid]->normals)[pqr.y];
        vec3f nr = ((vec3f*)geom[gid]->normals)[pqr.z];
        vec3f normal = bary.x*np + bary.y*nq + bary.z*nr;

        rays->nx[i] = normal.x;
        rays->ny[i] = normal.y;
        rays->nz[i] = normal.z;
    }
    else
    {
        rays->nx[i] = rayhit.hit.Ng_x;
        rays->ny[i] = rayhit.hit.Ng_y;
        rays->nz[i] = rayhit.hit.Ng_z;
    }

    rays->tMax[i] = rayhit.ray.tfar;
    rays->term[i] = rayhit.hit.geomID;
    rays->t[i]    = rayhit.ray.tfar;
  }
}

// This is for testing the VKL device interface from C++.   It is equivalent to
// the TraceRays_TraceRays in renderer/TraceRays.ispc - a call made from C++ that
// bridges the gap from C++ to ISPC.

export void
IntelModel_Sample(uniform IntelModel_ispc& self, const uniform int n, float *uniform x, float *uniform y, float *uniform z, float *uniform d)
{
  if (self.nVolumes == 0) return;

  uniform VKLVolume volume   = (uniform VKLVolume)self.volumes[0]->volume;
  uniform VKLSampler sampler = (uniform VKLSampler)self.volumes[0]->sampler;

  foreach (i = 0 ... n)
  {
    vkl_vec3f vklp = {x[i], y[i], z[i]};
#if 1
    d[i] = vklComputeSampleV((VKLSampler)sampler, &vklp);
#endif
  }
}

export void
IntelModel_IsoCrossing(uniform IntelModel_ispc& self, const uniform int n, void *uniform _rays, uniform int nv, float *uniform v)
{
  RTCScene scene = self.scene;
  EmbreeGeometry_ispc **uniform geom = self.geometries;
  uniform RayList_ispc *uniform rays = (uniform RayList_ispc *)_rays;
  RTCRayHit rayhit;

  uniform VKLVolume volume   = (uniform VKLVolume)self.volumes[0]->volume;
  uniform VKLSampler sampler = (uniform VKLSampler)self.volumes[0]->sampler;

  VKLValueSelector selector = vklNewValueSelector(volume);
  vklValueSelectorSetValues(selector, nv, v);
  vklCommit((VKLObject)selector);

  uniform unsigned int8 bufferHit[VKL_MAX_HIT_ITERATOR_SIZE];

  foreach (i = 0 ... n)
  {
    uniform RTCIntersectContext context;
    rtcInitIntersectContext(&context);

    vkl_vec3f o = {rays->ox[i], rays->oy[i], rays->oz[i]};
    vkl_vec3f d = {rays->dx[i], rays->dy[i], rays->dz[i]};
    vkl_range1f t = {rays->t[i], rays->tMax[i]};

    float time;

    VKLHitIterator hitIterator = vklInitHitIteratorV(sampler, &o, &d, &t, selector, bufferHit);
    VKLHit hit;
    vklIterateHitV(hitIterator, &hit);

    rays->tMax[i] = hit.t;
    print("t %\ntsample %\nepsilon %\n", hit.t, hit.sample, hit.epsilon);
  }
}
