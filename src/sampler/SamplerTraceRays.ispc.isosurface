// ========================================================================== //
// Copyright (c) 2016-2018 The University of Texas at Austin.                 //
// All rights reserved.                                                       //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// A copy of the License is included with this software in the file LICENSE.  //
// If your copy does not contain the License, you may obtain a copy of the    //
// License at:                                                                //
//                                                                            //
//     https://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  //
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
//                                                                            //
// ========================================================================== //

// adapted from Intel OSPRay ospray/render/scivis/SciVisRenderer.ispc
// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "galaxy.h"

#include "ospray/SDK/math/vec.ih"
#include "ospray/SDK/common/Ray.ih"
#include "ospray/SDK/render/util.ih"
#include "ospray/SDK/common/Model.ih"
#include "ospray/SDK/transferFunction/LinearTransferFunction.ih"
#include "ospray/SDK/volume/Volume.ih"

#include "Rays.ih"
#include "Lighting.ih"
#include "RayFlags.h"
#include "Visualization.ih"
#include "SamplerVis.ih"
#include "SamplerTraceRays.ih"

export void SamplerTraceRays_initialize(void *uniform _self)
{
  uniform SamplerTraceRays_ispc *uniform self = (uniform SamplerTraceRays_ispc *)_self;
}

export void SamplerTraceRays_destroy(void *uniform ispc)
{
}

inline float
EntryT(const Ray& ray, const uniform box3f& box)
{
  const vec3f mins = (box.lower - ray.org) * rcp(ray.dir);
  const vec3f maxs = (box.upper - ray.org) * rcp(ray.dir);

  return max(min(mins.x,maxs.x),
           max(min(mins.y,maxs.y),
               min(mins.z,maxs.z)));
}

inline float
ExitT(const Ray& ray, const uniform box3f& box)
{
  const vec3f mins = (box.lower - ray.org) * rcp(ray.dir);
  const vec3f maxs = (box.upper - ray.org) * rcp(ray.dir);

  // print("mins.x %\nmins.y %\nmins.z %\n", mins.x, mins.y, mins.z);
  // print("maxs.x %\nmaxs.y %\nmaxs.z %\n", maxs.x, maxs.y, maxs.z);

  return min(max(mins.x,maxs.x),
           min(max(mins.y,maxs.y),
               max(mins.z,maxs.z)));
}

// the ray origin should be inside the box.   Does the step also end
// inside the box?  If so, bump the origin by the step size and keep
// the containing loop going.   If not, figure out the exit point and
// cause the loop to break

inline bool
Step(Ray& ray, const uniform float step, const uniform box3f& box)
{
  vec3f coord = ray.org + step*ray.dir;

  if (box.lower.x > coord.x || box.lower.y > coord.y || box.lower.z > coord.z ||
      box.upper.x < coord.x || box.upper.y < coord.y || box.upper.z < coord.z) 
  {
    float tExit = ExitT(ray, box);
    if (tExit < 0 || tExit > step)

    ray.org  = ray.org + tExit*ray.dir;
    return true;
  }
  else
  {
    ray.org = coord;
    return false;
  }
}

export void* uniform SamplerTraceRays_SamplerTraceRays(void *uniform _self,
                               void *uniform _vis,
                               const uniform int nRaysIn,
                               void *uniform _raysIn)
{ 
  uniform SamplerTraceRays_ispc *uniform self = (uniform SamplerTraceRays_ispc *)_self;
  uniform Visualization_ispc *uniform vis = (uniform Visualization_ispc *)_vis;
  uniform RayList_ispc *uniform raysIn = (uniform RayList_ispc *)_raysIn;
  uniform box3f box = vis->local_bb;
  uniform float step;

  // For the moment, assume that the visualization contains only one vis and its a SampleVis

  uniform SamplerVis_ispc *uniform svis = (uniform SamplerVis_ispc *uniform)vis->volumeVis[0];
  
  if (vis->nVolumeVis == 1)
  {
    uniform Volume *uniform vol = (uniform Volume *uniform)((uniform Vis_ispc *uniform)svis)->data;

    uniform float isoval = svis->tolerance;
    uniform float step = vol->samplingStep * vol->samplingRate;

    foreach (i = 0 ... nRaysIn)
    {
      Ray ray;

      ray.org = make_vec3f(raysIn->ox[i], raysIn->oy[i], raysIn->oz[i]);
      ray.dir = make_vec3f(raysIn->dx[i], raysIn->dy[i], raysIn->dz[i]);
      ray.t = raysIn->t[i];

      if (ray.dir.x == 0.f) ray.dir.x = 1e-6f;
      if (ray.dir.y == 0.f) ray.dir.y = 1e-6f;
      if (ray.dir.z == 0.f) ray.dir.z = 1e-6f;

      // Get the volume enter/exit t's for the ray irrespective of the rays lifetime
      // tExitVolume is the distance to the exit boundary.  CANNOT be used for much
      // since the ray bends.

      float tEntry = EntryT(ray, box);
      float tExit = ExitT(ray, box);

      // print("tEntry %\ntExit %\nray.t %\nray.t0 %\n", tEntry, tExit, ray.t, ray.t0);

      // move the ray origin to the entry point (if its not already inside the volume)
      if (tEntry < ray.t)
        tEntry = ray.t;

      float tLast = tEntry;
      float tThis = tEntry + step;
      bool hit = false;

      vec3f coord = ray.org + tEntry * ray.dir;
      float sLast = vol->sample(vol, coord);
      // print("START T %\nZ %\nS %\n", tEntry, coord.z, sLast);

      while (tThis <= tExit && !hit)
      {
        coord = ray.org + tThis * ray.dir;
        float sThis = vol->sample(vol, coord);

        // print("T %\nZ %\nS %\n", tThis, coord.z, sThis);

        if (((sLast < isoval) && (sThis >= isoval)) || ((sLast > isoval) && sThis <= isoval))
        {
          // print ("hit!\n");
          tThis = tLast + (((isoval - sLast)/(sThis - sLast)) * (tThis - tLast));
          hit   = true;
        }
        else if (tThis == tExit)
        {
          break;
        }
        else
        {
          sLast = sThis;
          tLast = tThis;
          tThis = tThis + step;
          if (tThis > tExit)
            tThis = tExit;
        }
      }

      if (hit)
        raysIn->t[i] = tThis + 0.001;
      else
        raysIn->t[i] = tThis;

      raysIn->term[i] = hit ? RAY_SURFACE : RAY_BOUNDARY;
    }
  }
}

