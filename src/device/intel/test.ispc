#include "Rays.ih"
#include "IntelModel.ih"

export void
Test_Intersect(void *uniform _model, const uniform int nRays, void *uniform _rays)
{
  uniform IntelModel_ispc *uniform imodel = (uniform IntelModel_ispc *uniform)_model;
  uniform DeviceModel_ispc *uniform model = (uniform DeviceModel_ispc *uniform)imodel;
  RTCScene scene = imodel->scene; 

  EmbreeGeometry_ispc **uniform geom = imodel->geometries;
  uniform RayList_ispc *uniform rays = (uniform RayList_ispc *)_rays;

  GxyRayHit rayhit;
  GxySample sample;

  foreach (i = 0 ... nRays)
  { 
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
    
    model->IntersectSOA(1, _model, (void *)&rayhit, (void *)&sample);

    rays->nx[i]   = sample.normal.x;
    rays->ny[i]   = sample.normal.y;
    rays->nz[i]   = sample.normal.z;
    rays->tMax[i] = rayhit.ray.tfar;
    rays->term[i] = rayhit.hit.geomID < 0 ? 0 : 1;
    rays->t[i]    = rayhit.ray.tfar;
  }
}
