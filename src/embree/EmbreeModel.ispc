#include "embree3/rtcore_config.h"
#include "embree3/rtcore_ray.isph"
#include "embree3/rtcore.isph"

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
#error CRAP
#endif

void EmbreeModel_Intersect(RTCScene scene, void *uniform rayhit)
{
    uniform RTCIntersectContext context;
    rtcInitIntersectContext(&context);

    varying int valid = -1;
    INTERSECTOR((const int* uniform)&valid, scene, &context, (void *uniform)rayhit);
}

void EmbreeModel_postIntersect(void **uniform _normals, const uniform int n, void *uniform _rays)
{
}
