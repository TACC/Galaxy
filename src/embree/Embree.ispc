#include "embree3/rtcore_config.h"
#include "embree3/rtcore_ray.isph"
#include "embree3/rtcore.isph"

#include "Rays.ih"

// extern void EmbreeModel_Intersect(RTCScene scene, varying RTCRayHit * uniform rayhit);
// extern void EmbreeModel_Intersect(RTCScene scene, RTCRayHit* rayhit);
extern void EmbreeModel_Intersect(RTCScene scene, void *uniform rayhit);

export void Embree_Intersect(RTCScene scene, const uniform int n, void *uniform _rays)
{
    uniform RayList_ispc *uniform rays = (uniform RayList_ispc *)_rays;
    RTCRayHit rayhit;

    print("RAYHIT %\n", &rayhit);

    foreach (i = 0 ... n)
    {
        uniform RTCIntersectContext context;
        rtcInitIntersectContext(&context);

        print("%\n", i);

        rayhit.ray.org_x     = rays->ox[i];
        print("%\n", rayhit.ray.org_x);
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

        print("%\n", rayhit.ray.org_x);

        EmbreeModel_Intersect(scene, (void *)&rayhit);

        print("hit? %\n", rayhit.hit.geomID);

        rays->term[i] = rayhit.hit.geomID;
        rays->t[i]    = rayhit.ray.tfar;
    }
}

export void Embree_postIntersect(void **uniform _normals, const uniform int n, void *uniform _rays)
{
}
