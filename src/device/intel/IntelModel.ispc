#include "embree3/rtcore_config.h"
#include "embree3/rtcore_ray.isph"
#include "embree3/rtcore.isph"

#include "IntelModel.ih"
#include "EmbreeGeometry.ih"
#include "Rays.ih"

export void
IntelModel_IntersectSOA(uniform bool shadeFlag, void *uniform _imodel, void *uniform _rayhit, void *uniform _sample)
{
  uniform IntelModel_ispc *uniform imodel = (uniform IntelModel_ispc*)_imodel;
  RTCScene scene = imodel->scene;
  uniform RTCIntersectContext context;
  rtcInitIntersectContext(&context);

  GxySample *sample = (GxySample *)_sample;
  GxyRayHit *rayhit = (GxyRayHit *)_rayhit;

  rtcIntersectV(scene, &context, (varying RTCRayHit *uniform)_rayhit);

  if(rayhit->hit.geomID < 0)
  {
    return;
  }
  else
  {
    if (rayhit->hit.geomID >= imodel->nGeometries)
    {
      print("UhOh 0!\n");
      print("R %\n", rayhit->hit.geomID);
      print("I %\n", imodel->nGeometries);
    }
    else if (shadeFlag)
    {
        // print("OK 0!\n");
        // print("R %\n", rayhit->hit.geomID);
        // print("I %\n", imodel->nGeometries);
        foreach_unique(gid in rayhit->hit.geomID)
          imodel->geometries[gid]->postIntersect(gid, _imodel, _rayhit, _sample);
    }
  }
}

// This is for testing the Embree device interface from C++.   It is equivalent to
// the TraceRays_TraceRays in renderer/TraceRays.ispc - a call made from C++ that
// bridges the gap from C++ to ISPC.

export void
IntelModel_Intersect(uniform IntelModel_ispc *uniform imodel, const uniform int n, void *uniform _rays)
{
  uniform DeviceModel_ispc *uniform model = (uniform DeviceModel_ispc *uniform)imodel;
  RTCScene scene = imodel->scene;
  EmbreeGeometry_ispc **uniform geom = imodel->geometries;
  uniform RayList_ispc *uniform rays = (uniform RayList_ispc *)_rays;
  GxyRayHit rayhit;
  GxySample sample;

  foreach (i = 0 ... n)
  {
    print("I %\n", i);
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

    // IntelModel_IntersectSOA(true, scene, (void *)&rayhit, (void *)&sample);
    model->IntersectSOA(true, scene, (void *)&rayhit, (void *)&sample);

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

