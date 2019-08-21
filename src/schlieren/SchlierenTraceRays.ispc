#define PRINT_BEND if (1 == 0)

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
#include "SchlierenTraceRays.ih"
#include "Lighting.ih"
#include "RayFlags.h"
#include "Visualization.ih"
#include "VolumeVis.ih"

export void SchlierenTraceRays_initialize(void *uniform _self)
{
}

export void SchlierenTraceRays_destroy(void *uniform ispc)
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

  PRINT_BEND print("mins.x %\nmins.y %\nmins.z %\n", mins.x, mins.y, mins.z);
  PRINT_BEND print("maxs.x %\nmaxs.y %\nmaxs.z %\n", maxs.x, maxs.y, maxs.z);

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

#if 0
  print("box\nl % % %\nu % % %\nOX %\nOY %\nOZ %\n", 
    box.lower.x,
    box.lower.y,
    box.lower.z,
    box.upper.x,
    box.upper.y,
    box.upper.z,
    coord.x, coord.y, coord.z);
#endif

  if (box.lower.x > coord.x || box.lower.y > coord.y || box.lower.z > coord.z ||
      box.upper.x < coord.x || box.upper.y < coord.y || box.upper.z < coord.z) 
  {
    float tExit = ExitT(ray, box);
    PRINT_BEND print("step outside tExit %\n", tExit);

    if (tExit < 0 || tExit > step)
       PRINT_BEND print("invalid tExit\n");

    ray.org  = ray.org + tExit*ray.dir;
    PRINT_BEND print("Exit: ", ray.org.z);

    return true;
  }
  else
  {
    ray.org = coord;
    return false;
  }
}

export void* uniform SchlierenTraceRays_SchlierenTraceRays(void *uniform _self,
                               void *uniform _vis,
                               const uniform int nRaysIn,
                               void *uniform _raysIn)
{ 
  uniform SchlierenTraceRays_ispc *uniform self = (uniform SchlierenTraceRays_ispc *)_self;
  uniform Visualization_ispc *uniform vis = (uniform Visualization_ispc *)_vis;
  uniform RayList_ispc *uniform raysIn = (uniform RayList_ispc *)_raysIn;
  uniform box3f box = vis->local_bb;
  uniform float step;

  // print("AT TOP: mask %\n", __mask);

  PRINT_BEND print("box\nl % % %\nu % % %\n",
    box.lower.x,
    box.lower.y,
    box.lower.z,
    box.upper.x,
    box.upper.y,
    box.upper.z);

  if (vis->nVolumeVis == 1)
  {
    uniform VolumeVis_ispc *uniform vvis = vis->volumeVis[0];
    uniform Volume *uniform vol = (uniform Volume *uniform)((uniform Vis_ispc *uniform)vvis)->data;

    step = vol->samplingStep * vol->samplingRate;

    foreach (i = 0 ... nRaysIn)
    {
      Ray ray;

      ray.org = make_vec3f(raysIn->ox[i], raysIn->oy[i], raysIn->oz[i]);
      ray.dir = make_vec3f(raysIn->dx[i], raysIn->dy[i], raysIn->dz[i]);

      ray.dir = normalize(ray.dir);

      PRINT_BEND print("in===================\n");
      PRINT_BEND print("OX %\nOY %\nOZ %\n", ray.org.x, ray.org.y, ray.org.z);
      PRINT_BEND print("DX %\nDY %\nDZ %\n", ray.dir.x, ray.dir.y, ray.dir.z);
      PRINT_BEND print("IOX %\nIOY %\nIOZ %\n", ray.org.x, ray.org.y, ray.org.z);
      PRINT_BEND print("IDX %\nIDY %\nIDZ %\n", ray.dir.x, ray.dir.y, ray.dir.z);

      if (ray.dir.x == 0.f) ray.dir.x = 1e-6f;
      if (ray.dir.y == 0.f) ray.dir.y = 1e-6f;
      if (ray.dir.z == 0.f) ray.dir.z = 1e-6f;

      // Get the volume enter/exit t's for the ray irrespective of the rays lifetime
      // tExitVolume is the distance to the exit boundary.  CANNOT be used for much
      // since the ray bends.

      float tEntry = EntryT(ray, box);
      PRINT_BEND print("tE: %\n", tEntry);

      // move the ray origin to the entry point (if its not already inside the volume)

      if (tEntry > 0.0) ray.org = ray.org + tEntry * ray.dir;

      float sLast = vol->sample(vol, ray.org);
      PRINT_BEND print("sL %\n", sLast);

      bool done = false;
      do
      {
        done = Step(ray, step, box);

        float sThis = vol->sample(vol, ray.org);
        PRINT_BEND print("sT %\n", sThis);

        // print("X %\nY %\nZ %\nS %\n", ray.org.x, ray.org.y, ray.org.z, sThis);

        if (sThis != sLast)
        {
          vec3f grad = vol->computeGradient(vol, ray.org);

          float magg = length(grad);
          PRINT_BEND print("GX %\nGY %\nGZ %\n", grad.x, grad.y, grad.z);

          if (magg > 0.0001)
          {

            grad = normalize(grad);
            PRINT_BEND print("GX %\nGY %\nGZ %\n", grad.x, grad.y, grad.z);

            PRINT_BEND print("DX %\nDY %\nDZ %\n", ray.dir.x, ray.dir.y, ray.dir.z);
            PRINT_BEND print("SL %\nST %\n", sLast, sThis);

            // Axis of rotation
            vec3f u = make_vec3f(ray.dir.y*grad.z - ray.dir.z*grad.y, ray.dir.z*grad.x - ray.dir.x*grad.z, ray.dir.x*grad.y - ray.dir.y*grad.x);

            float dcu = length(u);
            if (dcu < 0.001)
              continue;

            u = normalize(u);

            // print("UX %\nUY %\nUZ %\n", u.x, u.y, u.z);

            // Alpha is the angle between the incoming direction and the gradient, beta the angle between the
            // new outgoing direction and the gradient


            // print("DX %\nDY %\nDZ %\n", ray.dir.x, ray.dir.y, ray.dir.z);

            vec3f rdir = ray.dir;
            rdir = normalize(rdir);

            float cos_theta1 = dot(grad, rdir);
            if (cos_theta1 != 0.0)
            {
              if (cos_theta1 < 0.0) cos_theta1 = -cos_theta1;

              // print("CT1 %\n", cos_theta1);

              float theta1 = acos(cos_theta1);
              // print("T1 %\n", theta1);

              float bend = sLast / sThis;
              // print("SL %\nST %\nB %\n", sLast, sThis, bend);

              float theta2 = bend*theta1;
              float R = theta1 - theta2;

              // print("R %\n", R);

              float cosR = cos(R);
              float sinR = sin(R);
            
              vec3f vx = make_vec3f(cosR + u.x*u.x*(1 - cosR),     u.x*u.y*(1 - cosR) - u.z*sinR, u.x*u.z*(1 - cosR) + u.y*sinR);
              vec3f vy = make_vec3f(u.y*u.x*(1 - cosR) + u.z*sinR, cosR + u.y*u.y*(1 - cosR),     u.y*u.z*(1 - cosR) - u.x*sinR);
              vec3f vz = make_vec3f(u.z*u.x*(1 - cosR) - u.y*sinR, u.z*u.y*(1 - cosR) + u.x*sinR, cosR + u.z*u.z*(1 - cosR));

              vec3f rdir = (ray.dir.x * vx) + (ray.dir.y * vy) + (ray.dir.z * vz);
              rdir = normalize(rdir);
              ray.dir = rdir;
            }
          }
        }
        sLast = sThis;
      }
      while (!done);
      
      PRINT_BEND print("out===================\n");
      PRINT_BEND print("OX %\nOY %\nOZ %\n", ray.org.x, ray.org.y, ray.org.z);
      PRINT_BEND print("DX %\nDY %\nDZ %\n", ray.dir.x, ray.dir.y, ray.dir.z);

      raysIn->term[i] |= RAY_BOUNDARY;
      raysIn->ox[i] = ray.org.x;
      raysIn->oy[i] = ray.org.y;
      raysIn->oz[i] = ray.org.z;
      raysIn->dx[i] = ray.dir.x;
      raysIn->dy[i] = ray.dir.y;
      raysIn->dz[i] = ray.dir.z;
            
      PRINT_BEND print("FOX %\nFOY %\nFOZ %\n", ray.org.x, ray.org.y, ray.org.z);
      PRINT_BEND print("FDX %\nFDY %\nFDZ %\n", ray.dir.x, ray.dir.y, ray.dir.z);
    }
  }
  else
     PRINT_BEND print("Can only Schlieren one volume\n");
}

