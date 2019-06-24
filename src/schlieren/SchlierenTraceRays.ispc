#define PRINT_BEND  if (1 == 0)

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

   PRINT_BEND print("BuZ: %\n", box.upper.x);

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

  PRINT_BEND print("box\nl % % %\nu % % %\nOX %\nOY %\nOZ %\n", 
    box.lower.x,
    box.lower.y,
    box.lower.z,
    box.upper.x,
    box.upper.y,
    box.upper.z,
    coord.x, coord.y, coord.z);

  if (box.lower.x > coord.x || box.lower.y > coord.y || box.lower.z > coord.z ||
      box.upper.x < coord.x || box.upper.y < coord.y || box.upper.z < coord.z) 
  {
    float tExit = ExitT(ray, box);
    if (tExit < 0 || tExit > step)
       PRINT_BEND print("invalid tExit\n");

     PRINT_BEND print("XX: org.z = %\ndir.z = %\ntExit = %\n", ray.org.z, ray.dir.z, tExit);
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
      if (ray.dir.x == 0.f) ray.dir.x = 1e-6f;
      if (ray.dir.y == 0.f) ray.dir.y = 1e-6f;
      if (ray.dir.z == 0.f) ray.dir.z = 1e-6f;

      // Get the volume enter/exit t's for the ray irrespective of the rays lifetime
      // tExitVolume is the distance to the exit boundary.  CANNOT be used for much
      // since the ray bends.

      float tEntry = EntryT(ray, box);

      // move the ray origin to the entry point (if its not already inside the volume)

      if (tEntry > ray.t0) ray.org = ray.org + tEntry * ray.dir;

      float sLast = vol->sample(vol, ray.org);
      PRINT_BEND print("sL %\n", sLast);

      bool done = false;
      do
      {
        vec3f odir = normalize(ray.dir);

        done = Step(ray, step, box);
        PRINT_BEND print("sN % %\n", sLast, done);

        float sThis = vol->sample(vol, ray.org);

        if (sThis != sLast)
        {
          vec3f grad = vol->computeGradient(vol, ray.org);
          PRINT_BEND print("GX %\nGY %\nGZ %\n", grad.x, grad.y, grad.z);

          // Better not be zero!

          float magd = length(ray.dir);
          float magg = length(grad);

          if (magd > 0.0001 && magg > 0.0001)
          {
            PRINT_BEND print("OX %\nOY %\nOZ %\n", ray.org.x, ray.org.y, ray.org.z);

            vec3f nD = (1/magd) * ray.dir;
            PRINT_BEND print("DIX %\nDIY %\nDIZ %\n", nD.x, nD.y, nD.z);

            vec3f nG = (1/magg) * grad;
            PRINT_BEND print("nGX %\nnGY %\nnGZ %\n", nG.x, nG.y, nG.z);

            // Axis of rotation

            vec3f u = make_vec3f(nD.y*nG.z - nD.z*nG.y, nD.z*nG.x - nD.x*nG.z, nD.x*nG.y - nD.y*nG.x);
            PRINT_BEND print("UX %\nUY %\nUZ %\n", u.x, u.y, u.z);

            float dcu = length(u);
            if (dcu < 0.000001)
              continue;
       
            u = u / dcu;

            // Alpha is the angle between the incoming direction and the gradient, beta the angle between the
            // new outgoing direction and the gradient

            float cosAlpha = dot(nG, nD);
            if (cosAlpha > -0.000001 && cosAlpha < 0.000001)
              continue;

            float sinAlpha = cos2sin(cosAlpha);

            float bend = sLast / sThis;
            float sinBeta = bend * sinAlpha;
            float cosBeta = sin2cos(sinBeta);

            if (cosAlpha < 0)
              cosBeta = -cosBeta;

            PRINT_BEND print("BEND %\nSA %\nCA %\n", bend, sinAlpha, cosAlpha);
            PRINT_BEND print("SB %\nCB %\nUX %\nUY %\nUZ %\n", sinBeta, cosBeta, u.x, u.y, u.z);

            vec3f vx = make_vec3f(cosBeta + u.x*u.x*(1 - cosBeta),     u.x*u.y*(1 - cosBeta) - u.z*sinBeta, u.x*u.z*(1 - cosBeta) + u.y*sinBeta);
            vec3f vy = make_vec3f(u.y*u.x*(1 - cosBeta) + u.z*sinBeta, cosBeta + u.y*u.y*(1 - cosBeta),     u.y*u.z*(1 - cosBeta) - u.x*sinBeta);
            vec3f vz = make_vec3f(u.z*u.x*(1 - cosBeta) - u.y*sinBeta, u.z*u.y*(1 - cosBeta) + u.x*sinBeta, cosBeta + u.z*u.z*(1 - cosBeta));

            PRINT_BEND print("VXX %\nVXY %\nVXZ %\n", vx.x, vx.y, vx.z);
            PRINT_BEND print("VYX %\nVYY %\nVYZ %\n", vy.x, vy.y, vy.z);
            PRINT_BEND print("VZX %\nVZY %\nVZZ %\n", vz.x, vz.y, vz.z);

            vec3f rdir = (nG.x * vx) + (nG.y * vy) + (nG.z * vz);
            
            // DO NOT BEND   ...   DEBUGGING ray.dir = magd*rdir;

            vec3f ndir = normalize(ray.dir);
            float dirdot = dot(odir, ndir);

            PRINT_BEND print("DOX %\nDOY %\nDOZ %\nDD %\n", rdir.x, rdir.y, rdir.z, dirdot);
            PRINT_BEND print("done\n");

          }
        }
        sLast = sThis;
      }
      while (!done);
      
      raysIn->term[i] |= RAY_BOUNDARY;
      raysIn->ox[i] = ray.org.x;
      raysIn->oy[i] = ray.org.y;
      raysIn->oz[i] = ray.org.z;
      raysIn->dx[i] = ray.dir.x;
      raysIn->dy[i] = ray.dir.y;
      raysIn->dz[i] = ray.dir.z;
    }
  }
  else
     PRINT_BEND print("Can only Schlieren one volume\n");
}

