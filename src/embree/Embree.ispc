#include "embree3/rtcore.isph"

export void em_Intersect(RTCScene scene, int* uniform valid, void* uniform rays)
{
    uniform RTCIntersectContext context;
    rtcInitIntersectContext(&context);

    rtcIntersect8((const int* uniform)valid, scene, &context, (void *uniform) rays);
}

