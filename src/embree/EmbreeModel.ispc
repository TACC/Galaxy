#include "embree3/rtcore_config.h"
#include "embree3/rtcore_ray.isph"
#include "embree3/rtcore.isph"

#include "EmbreeGeometry.ih"
#include "Rays.ih"

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
EmbreeModel_IntersectSOA(RTCScene scene, void *uniform rayhit)
{
    uniform RTCIntersectContext context;
    rtcInitIntersectContext(&context);

    varying int valid = -1;
    INTERSECTOR((const int* uniform)&valid, scene, &context, rayhit);
}

// This is for testing the Embree device interface from C++.   It is equivalent to
// the TraceRays_TraceRays in renderer/TraceRays.ispc - a call made from C++ that
// bridges the gap from C++ to ISPC.

export void
EmbreeModel_Intersect(RTCScene scene,  EmbreeGeometry_ispc **uniform geom, const uniform int n, void *uniform _rays)
{
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

        EmbreeModel_IntersectSOA(scene, (void *)&rayhit);

        int gid = rayhit.hit.geomID;
        if (gid != RTC_INVALID_GEOMETRY_ID && geom[gid]->connectivity && geom[gid]->normals)
        {
            // print("GID %\n", gid);
            // print("PID %\n", rayhit.hit.primID);
            // print("X %\nY %\nZ %\n", rayhit.ray.org_x, rayhit.ray.org_y, rayhit.ray.org_z);
            vec3f bary = make_vec3f(1.0f - rayhit.hit.u - rayhit.hit.v, rayhit.hit.u, rayhit.hit.v);
            // print("BX %\nBY %\nBZ %\n", bary.x, bary.y, bary.z);
            vec3i pqr = ((vec3i*)geom[gid]->connectivity)[rayhit.hit.primID];
            // print("PQRX %\nPQRY %\nPQRZ %\n", pqr.x, pqr.y, pqr.z);
            vec3f np = ((vec3f*)geom[gid]->normals)[pqr.x];
            // print("NPX %\nNPY %\nNPZ %\n", np.x, np.y, np.z);
            vec3f nq = ((vec3f*)geom[gid]->normals)[pqr.y];
            // print("NQX %\nNQY %\nNQZ %\n", nq.x, nq.y, nq.z);
            vec3f nr = ((vec3f*)geom[gid]->normals)[pqr.z];
            // print("NRX %\nNRY %\nNRZ %\n", nr.x, nr.y, nr.z);
            vec3f normal = bary.x*np + bary.y*nq + bary.z*nr;
            rays->nx[i] = normal.x;
            rays->ny[i] = normal.y;
            rays->nz[i] = normal.z;
        }

        rays->term[i] = rayhit.hit.geomID;
        rays->t[i]    = rayhit.ray.tfar;
    }
}
