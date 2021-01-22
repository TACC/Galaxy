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

// distantly descended from Intel OSPRay ospray/render/scivis/SciVisRenderer.ispc

// ======================================================================== //
//                                                                          //
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
//                                                                          //
// ======================================================================== //

#if 1
#include "Rays.ih"
export void *uniform RayList_dummy(RayList_ispc *uniform a) {}
#else

#include "galaxy.h"

#include "embree3/rtcore_config.h"
#include "embree3/rtcore_ray.isph"
#include "embree3/rtcore.isph"

#include "vec.ih"

#include "hits.h"
#include "Lighting.ih"
#include "RayFlags.h"
#include "Rays.ih"
#include "TraceRays.ih"
#include "Visualization.ih"
#include "VolumeVis.ih"

#include "UV.ih"

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

export void *uniform TraceRays_allocate()
{
    TraceRays_ispc *uniform v = uniform new uniform TraceRays_ispc;
    return (void *)v;
}


export void TraceRays_initialize(void *uniform _self)
{
  uniform TraceRays_ispc *uniform self = (uniform TraceRays_ispc *)_self;

  for (uniform i = 0; i < 100; i++)
    self->debug[i] = -1;
}



export void TraceRays_destroy(void *uniform ispc)
{
}

struct Hit
{
  float sample;
  float t;

  vec3f point;
  vec3f normal;
  vec3f color;
  float opacity;
};

inline void
MyIntersectBox(const RTCRayHit& ray,
               const uniform box3f& box,
               float& t0,
               float& t1)
{
  vec3f org = make_vec3f(ray.ray.org_x, ray.ray.org_y, ray.ray.org_z);
  vec3f dir = make_vec3f(ray.ray.dir_x, ray.ray.dir_y, ray.ray.dir_z);

  const vec3f mins = (box.lower - org) * rcp(dir);
  const vec3f maxs = (box.upper - org) * rcp(dir);

  t0 = max(min(mins.x,maxs.x),
           max(min(mins.y,maxs.y),
               min(mins.z,maxs.z)));

  t1 = min(max(mins.x,maxs.x),
           min(max(mins.y,maxs.y),
               max(mins.z,maxs.z)));
}

inline void
traceRay(uniform RTCScene scene, void *uniform rayhit)
{
  uniform RTCIntersectContext context;
  rtcInitIntersectContext(&context);

  varying int valid = -1;
  INTERSECTOR((const int* uniform)&valid, scene, &context, rayhit);
}

inline bool
LookForGeometryHit(varying bool shadeFlag,
                   uniform Model *uniform model,
                   varying RTCRayHit &ray,
                   uniform Visualization_ispc *uniform vis,
                   varying Hit &hit)
{
  //print("LFGH %\n", model);

  
  traceRay(model, ray);

  if(ray.geomID < 0)
  {
    return false;
  }
  else
  {
    if (shadeFlag)
    {
      DifferentialGeometry dg;

      postIntersect(model, dg, ray, DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD|DG_COLOR|DG_MATERIALID|DG_TEXCOORD);

      hit.color   = make_vec3f(dg.color.x, dg.color.y, dg.color.z);

      hit.opacity = dg.color.w;
      hit.point   = dg.P;
      hit.normal  = dg.Ns;
      hit.t       = ray.t;
    }

    return true;
  }
}

inline bool
LookForSliceHit(uniform int *uniform debug,
                varying bool shadeFlag,
                varying Ray &ray, 
                uniform Visualization_ispc *uniform vis,
                varying Hit &hit)
{
  bool h = false;
  int vid = -1, sid = -1;

  debug[6] = 0;

  if (vis->nVolumeVis > 0)
  {
    debug[6] = 1;
    debug[7] = vis->nVolumeVis;

    for (uniform  int major = 0; major < vis->nVolumeVis; major++)      // which volume?
    {
      uniform VolumeVis_ispc *uniform vvis = vis->volumeVis[major];
      debug[8] = 1;
      debug[9] = major;
      debug[10] = vvis->nSlices;

      for (uniform int minor = 0; minor < vvis->nSlices; minor ++)
      {
        debug[11] = 1;

        uniform vec4f plane = vvis->slices[minor];
        debug[11] = 2;

        vec3f pnorm = make_vec3f(plane.x, plane.y, plane.z);

        float d = plane.w;

        float denom = dot(ray.dir, pnorm);

        debug[11] = 3;
        if (abs(denom) > 0.0001)
        {
          debug[12] = 1;

          float t = (d - dot(ray.org, pnorm)) / denom;
          if (t >= ray.t0 && t <= ray.t)
          {
            debug[13] = 1;

            if (denom > 0)
              hit.normal   = neg(pnorm);
            else
              hit.normal   = pnorm;

            hit.opacity  = 1.0;       // FIXME
            hit.t        = t;
            vid          = major;
            sid          = minor;
            ray.t        = hit.t;
            h            = true;

            debug[13] = 2;
          }
        }
      }
    }
  }

  debug[6] = 2;

  if (h)
  {
    debug[14] = 1;

    if (shadeFlag)
    {
      debug[15] = 1;

      float s[100];
      hit.point = ray.org + hit.t * ray.dir;
 
      for (uniform int major = 0; major <= vid; major++)
        if (major == vid)
        {
          debug[16] = 1;
          uniform VolumeVis_ispc *uniform vvis = (VolumeVis_ispc *)vis->volumeVis[major];
          debug[16] = 2;
          uniform TransferFunction *uniform tf = (uniform TransferFunction *uniform )((MappedVis_ispc *)vvis)->transferFunction;
          debug[16] = 3;
          uniform Volume *uniform vol = (uniform Volume *uniform)(((Vis_ispc *)vvis)->data);
          debug[16] = 4;
          if (! vol)
            print("VOL IS ZERO!\n");
          hit.sample = vol->sample(vol, hit.point);
          debug[16] = 5;
          if (! tf) 
            print("TF is ZERO!\n");
          hit.color = tf->getColorForValue(tf, hit.sample);
          debug[16] = 6;
          hit.opacity = 1.0;
        }

        debug[15] = 2;
    }
  }

  debug[6] = 2;

  return h;
}

inline bool
LookForIsoHit(varying bool shadeFlag,
              varying Ray &ray, 
              varying float* sLast, varying float* sThis, // Last and current samples
              varying float tLast, varying float tThis,    // Last and current T
              uniform Visualization_ispc *uniform vis,
              varying Hit &hit)
{
  bool h = false;
  int vid;

  for (uniform int major = 0; major < vis->nVolumeVis; major++)      // which volume?
  {
    uniform VolumeVis_ispc *uniform vvis = vis->volumeVis[major];
    uniform Volume *uniform vol = (uniform Volume *uniform)(((Vis_ispc *)vvis)->data);

    float sl = sLast[major];
    float st = sThis[major];

    for (uniform int minor = 0; minor < vvis->nIsovalues; minor++) 
    {
      uniform float isoval = vvis->isovalues[minor];
      if (((isoval >= sl) && (isoval < st)) || ((isoval <= sl) && (isoval > st)))
      {
        h          = true;
        vid        = major;
        hit.t      = tLast + ((isoval - sl) / (st - sl)) * (tThis - tLast);
        hit.sample = isoval;
      }
    }
  }
  
  if (h)
  {
    hit.point = ray.org + hit.t * ray.dir;

    // print("hit\nT %\nX %\nY %\nZ %\n", hit.t, hit.point.x, hit.point.y, hit.point.z);

    if (shadeFlag)
    {
      for (uniform int major = 0; major <= vid; major++)
        if (major == vid)
        {
          uniform VolumeVis_ispc *uniform vvis = vis->volumeVis[major];
          uniform Volume *uniform vol = (uniform Volume *uniform)(((Vis_ispc *)vvis)->data);
          uniform TransferFunction *uniform tf = (uniform TransferFunction *uniform )((MappedVis_ispc *)vvis)->transferFunction;

          hit.normal = safe_normalize(vol->computeGradient(vol, hit.point));
          if (dot(ray.dir, hit.normal) > 0) 
            hit.normal = neg(hit.normal);

          hit.color = tf->getColorForValue(tf, hit.sample);
          hit.opacity = 1.0;
        }
    }
  }

  return h;
}

inline void
SampleVolumes(const varying vec3f& coord,
              uniform Visualization_ispc *uniform vis,
              varying float *s)
{ 
  bool h = false;
  for (uniform int major = 0; major < vis->nVolumeVis; major++)     // which volume?
  {
    uniform VolumeVis_ispc *uniform vvis = vis->volumeVis[major];
    uniform Volume *uniform vol = (uniform Volume *uniform)(((Vis_ispc *)vvis)->data);
    s[major] = vol->sample(vol, coord);
  }
}

export void *uniform TraceRays_TraceRays(void *uniform _self,
                               void *uniform _vis,
                               const uniform int nRaysIn,
                               void *uniform _raysIn, uniform float global_epsilon)
{ 
  uniform TraceRays_ispc *uniform self = (uniform TraceRays_ispc *)_self;
  uniform Visualization_ispc *uniform vis = (uniform Visualization_ispc *)_vis;
  uniform RayList_ispc *uniform raysIn = (uniform RayList_ispc *)_raysIn;
  Model *uniform model = (Model *uniform) vis->model;
  uniform box3f box = vis->local_bb;
  uniform bool integrate = false;
  uniform float step, epsilon;
  uniform Camera *uniform c;

  self->debug[0] = 1;

  step = -1;
  for (uniform int major = 0; major < vis->nVolumeVis; major++)
  {
    uniform VolumeVis_ispc *uniform vvis = vis->volumeVis[major];
    uniform Volume *uniform vol = (uniform Volume *uniform)((uniform Vis_ispc *uniform)vvis)->data;
 
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
  }

  self->debug[1] = nRaysIn;

  uniform int II = 0;
  foreach (i = 0 ... nRaysIn)
  {
    self->debug[0] = 2;
    self->debug[2] = II++;

    bool shadeFlag = raysIn->type[i] == RAY_PRIMARY;

    RTCRayHit ray;

    ray.org = make_vec3f(raysIn->ox[i], raysIn->oy[i], raysIn->oz[i]);
    ray.dir = make_vec3f(raysIn->dx[i], raysIn->dy[i], raysIn->dz[i]);
    if (ray.dir.x == 0.f) ray.dir.x = 1e-6f;
    if (ray.dir.y == 0.f) ray.dir.y = 1e-6f;
    if (ray.dir.z == 0.f) ray.dir.z = 1e-6f;

    ray.geomID = -1;
    ray.primID = -1;
    ray.instID = -1;
    ray.t0  = raysIn->t[i];
    ray.t   = raysIn->tMax[i];
    ray.time = 0.f;
    ray.mask = -1;

    float tTimeout = ray.t;

    vec4f color  = make_vec4f(raysIn->r[i], raysIn->g[i], raysIn->b[i], raysIn->o[i]);
    
    // tExit is the initial termination of the ray - infinity, or the AO radius
    // Its used to determine if loop termination was due to timeout

    varying float tExit  = ray.t;

    // Get the volume enter/exit t's for the ray irrespective of the rays lifetime
    // tExitVolume is the distance to the exit boundary.  Used to determine if loop
    // termination was due to exitting the volume - which MAY terminate the ray or
    // may cause it to be traced into the next partition of a partitioned dataset.
    // The entry can potentially be negative if the ray origin is inside the volume 

    self->debug[0] = 3;

    float tEntry, tExitVolume;
    MyIntersectBox(ray, box, tEntry, tExitVolume);

    // If the volume entrypoint is beyong the current ray point, move the ray point up to
    // the entry point

    if (tEntry < ray.t0) tEntry = ray.t0;
    else if (tEntry > ray.t0) ray.t0 = tEntry;

    // And the loop termination point is intially the ray termination (if that comes before
    // tExitVolume) or tExitVolume.

    ray.t = min(ray.t, tExitVolume);

    // The Hit information is used to retain info if and when we hit a surface (slice, 
    // iso or geometry)

    Hit hit;          

    // Look for slice or geometry that stops the ray inside this block.  May continue
    // if its not opaque; but that'll be in a later call to this routine after we've
    // cast the secondary rays  These two routines update the ray's t if something is
    // found.

    self->debug[0] = 4;
    self->debug[5] = 0;

    bool surface_hit = LookForSliceHit(self->debug, shadeFlag, ray, vis, hit);

    self->debug[5] = 1;

    if (model)
      surface_hit |= LookForGeometryHit(shadeFlag, model, ray, vis, hit);

    self->debug[5] = 2;

    float tTermination = ray.t;   // May be partition exit point or point at which slice/geometry is encountered

    self->debug[0] = 5;

    if (integrate)
    {
      self->debug[0] = 6;
      self->debug[3] = 0;

      // Now iterate up the interval from tEntry to tTermination, accruing volume contributions and looking for
      // isosurfaces

      varying float tLast, tThis;
      float sLast[100], sThis[100];   // last and current samples for up to 100 volumes (should be dynamic)

      // Note complicated step - last step to boundary must be included, but step AFTER
      // THAT one must not.

      bool hit_isosurface = false;

      tLast = tEntry + epsilon;

      int iterations = 0;

      bool opaque = (min(min(color.x, color.y), color.z) >= 1.0f || color.w > 0.999f);

      self->debug[3] = 1;

      for (tThis = tEntry;
           tThis <= tTermination && !opaque && !hit_isosurface; 
           tThis = (tThis == tEntry) ? (tEntry + epsilon) : (((tThis + step) > tTermination) && (tThis < tTermination)) ? tTermination : tThis + step)
      {
        self->debug[4] = 1;

        iterations = iterations + 1;

        vec3f coord = ray.org + tThis * ray.dir;

        SampleVolumes(coord, vis, sThis);
        self->debug[4] = 2;

        // Need to adjust interval for any iso hit before integrating for volume rendering so
        // we search interval for iso hit first.
        //
        // Only look for an isosurface if this is NOT the first iteration.   If an isosurface is hit, tThis and sThis will be reset to
        // reflect the interpolation within the step.  tTermination is then moved up to tThis since we need to bail out of the loop at
        // the surface hit.   Note the second condition - this is because of AO and shadow rays cast from an isosurface hit.   Since the
        // hit point is linearly interpolated, it might actually be a bit beyond the actual isosurface, so this is a second bulwark against 
        // re-hitting the isosurface immediately.

        if (tThis > tEntry && tLast >= epsilon)
        {
          self->debug[4] = 3;

          if (LookForIsoHit(shadeFlag, ray, sLast, sThis, tLast, tThis, vis, hit))
          {
            tTermination = hit.t;     // Terminate both the ray and the interval at the hit point 
            tThis = hit.t;            

            surface_hit = true;
            hit_isosurface = true;

            vec3f coord = ray.org + tThis * ray.dir;
            SampleVolumes(coord, vis, sThis);
          }

          self->debug[4] = 5;

          // Go through volumes accumulating opacity

          for (uniform int major = 0; major < vis->nVolumeVis; major++)      // which volume?
          {
            uniform VolumeVis_ispc *uniform vvis = vis->volumeVis[major];

            if (vvis->volume_render)
            {
              uniform Volume *uniform vol = (uniform Volume *uniform)((uniform Vis_ispc *uniform)vvis)->data;
              uniform TransferFunction *uniform tf = (uniform TransferFunction *uniform )vol->transferFunction;

              float sVolume = (sLast[major] + sThis[major]) / 2;
              float sampleOpacity = tf->getOpacityForValue(tf, sVolume);

              if (sampleOpacity > 0)
              {

                if (shadeFlag)
                {
									vec3f sampleColor = tf->getColorForValue(tf, sVolume);
									float wo = clamp(sampleOpacity / vol->samplingRate);
                  vec4f weightedColor = wo * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);
                  color = color + (1.0f - color.w) * weightedColor;
                }
                else
                {
								  float weightedOpacity = ((tThis - tLast) / step) * clamp(sampleOpacity / vol->samplingRate);
                  color = color * (1-weightedOpacity);
                }
              }
            }
            sLast[major] = sThis[major];
          }
        }

        self->debug[4] = 6;

        for (uniform int major = 0; major < vis->nVolumeVis; major++)  
          sLast[major] = sThis[major];

        opaque = (min(min(color.x, color.y), color.z) >= 1.0f || color.w > 0.999f);
        if (opaque) 
          tTermination = tThis;
        
        tLast = tThis;
        self->debug[4] = 7;
      }

      // To get the above loop to terminate correctly at the termination T, the last valid step is ending T
      // is clamped to the termination T and the subsequent step goes PAST there, though its invalid.
      // So if tThis is beyond the termination T we back it up.

      if (tThis > tTermination) 
        tThis = tTermination;

      // The above loop may have terminated early due to a) hitting isosurface or b) accumulating opacity, in
      // in which case the tTermination is bumped up.   So we reset ray's t

      ray.t = tTermination;
      self->debug[4] = 8;
    }

    self->debug[0] = 7;

    raysIn->r[i] = color.x;
    raysIn->g[i] = color.y;
    raysIn->b[i] = color.z;
    raysIn->o[i] = color.w;

    // Does the ray terminate with full opacity?

    raysIn->term[i] = (min(min(color.x, color.y), color.z) >= 1.0f || color.w > 0.999f) ? RAY_OPAQUE : 0;
    raysIn->t[i]    = ray.t;

    // Is there a reason the ray terminated OTHER THAN OR IN ADDITION TO opacity?

    self->debug[0] = 8;

    if (surface_hit)
    {
      raysIn->term[i] |= RAY_SURFACE;
      if (hit.opacity > 0.999)
        raysIn->term[i] |= RAY_OPAQUE;

      // Save surface info for later shading...

      raysIn->sr[i] = hit.color.x;
      raysIn->sg[i] = hit.color.y;
      raysIn->sb[i] = hit.color.z;
      raysIn->so[i] = 1.0;

      raysIn->nx[i] = hit.normal.x;
      raysIn->ny[i] = hit.normal.y;
      raysIn->nz[i] = hit.normal.z;
    }
    else if (tTermination == tExitVolume)
    {
      raysIn->term[i] |= RAY_BOUNDARY;
    }
    else if (tTermination == tTimeout)
    {
      raysIn->term[i] |= RAY_TIMEOUT;
    }
    else if (raysIn->term[i] != RAY_OPAQUE)
    {
      // print("TE term %\nX %\nY %\n", raysIn->term[i], raysIn->x[i], raysIn->y[i]);
      print("TERMINATION ERROR\n");
    }

    self->debug[0] = 9;
  }

  self->debug[0] = 10;
}

export void TraceRays_generateAORays(void *uniform _self,
                                void *uniform uniform _lights,
                                const uniform int nRaysIn,
                                void *uniform _raysIn,
                                uniform int *uniform offsets,
                                void *uniform _raysOut, uniform float global_epsilon)
                                
{
  TraceRays_ispc *uniform self = (TraceRays_ispc *uniform)_self;
  Lighting_ispc *uniform lights = (Lighting_ispc *uniform)_lights;
  uniform RayList_ispc *uniform raysIn  = (RayList_ispc *uniform)_raysIn;
  uniform RayList_ispc *uniform raysOut = (RayList_ispc *uniform)_raysOut;
  uniform float epsilon = global_epsilon;

#ifdef GXY_REVERSE_LIGHTING
  uniform float Ka = -lights->Ka / lights->n_ao_rays;
#else
  uniform float Ka = lights->Ka / lights->n_ao_rays;
#endif

  for (int i = 0; i < nRaysIn; i++)
  {
    // Only cast secondary rays for primary rays that hit a surface
    if ((raysIn->type[i] == RAY_PRIMARY) && (raysIn->term[i] & RAY_SURFACE))
    {
      int t = raysIn->term[i];

      // Initially, the input ray color is the accumulated result up to the current
      // surface.   If there is, add in the lighted surface color either by casting
      // AO and/or shadow rays, or just light the sucker here.  In any event, the
      // contribution of the surface is attenuated by the current opacity of the primary
      // ray.

      // Going to need the surface normal regardless of whether ambient and diffuse are 
      // added by secondary rays or right here.
        
      vec3f surface_normal = make_vec3f(raysIn->nx[i], raysIn->ny[i], raysIn->nz[i]);

      float ambient_scale = Ka * (1.0 - raysIn->o[i]);

      float ambient_r = ambient_scale * raysIn->sr[i];
      float ambient_g = ambient_scale * raysIn->sg[i];
      float ambient_b = ambient_scale * raysIn->sb[i];

      vec3f b0 = make_vec3f(1.0f, 0.0f, 0.0f);
      if (abs(dot(b0, surface_normal)) > 0.95) b0 = make_vec3f(0.0f, 1.0f, 0.0f);
      vec3f b1 = normalize(cross(b0, surface_normal));
      b0 = normalize(cross(b1, surface_normal));

      float ox = raysIn->ox[i] + raysIn->t[i]*raysIn->dx[i];
      float oy = raysIn->oy[i] + raysIn->t[i]*raysIn->dy[i];
      float oz = raysIn->oz[i] + raysIn->t[i]*raysIn->dz[i];

      ox = ox + epsilon*surface_normal.x;
      oy = oy + epsilon*surface_normal.y;
      oz = oz + epsilon*surface_normal.z;

      foreach (j = 0 ... lights->n_ao_rays)
      {
        int offset = offsets[i] + j;

				// For those who don't immediately recognize code that generates an 8-bit
				// pseudo-random number tied to a pixel location and particular AO ray index at that
				// pixel location, the following line of code generates an 8-bit pseudo-random
        // number tied to a pixel location and particular AO ray index at that
        // pixel location

        int r = ((raysIn->x[i] * 9949 + raysIn->y[i] * 9613 + j*9151)>>8) & 0xff;

        const float r0 = randomU[r];
        const float r1 = randomV[r];

        const float w = sqrt(1.f-r1);
#if 1
        const float x = cos((2.f*M_PI)*r0)*w;
        const float y = sin((2.f*M_PI)*r0)*w;
        const float z = sqrt(r1)+epsilon;
#else
        const float x = 0;
        const float y = 0;
        const float z = 1;
#endif

        vec3f rd = x*b0 + y*b1 + z*surface_normal;
        // vec3f rd = surface_normal;
      
        const float d = 1.0 / lights->n_ao_rays;
        
        raysOut->ox[offset]    = ox;
        raysOut->oy[offset]    = oy;
        raysOut->oz[offset]    = oz;

        raysOut->dx[offset]    = rd.x;
        raysOut->dy[offset]    = rd.y;
        raysOut->dz[offset]    = rd.z;
        raysOut->r[offset]     = ambient_r;
        raysOut->g[offset]     = ambient_g;
        raysOut->b[offset]     = ambient_b;
        raysOut->o[offset]     = 0.0;
        raysOut->t[offset]     = 0.0;
        raysOut->tMax[offset]  = lights->ao_radius;
        raysOut->x[offset]     = raysIn->x[i];
        raysOut->y[offset]     = raysIn->y[i];
        raysOut->type[offset]  = RAY_AO;
        raysOut->term[offset]  = 0;
      }
    }
  }
}

export void TraceRays_ambientLighting(void *uniform _self,
                               void *uniform uniform _lights,
                               const uniform int nRaysIn,
                               void *uniform uniform _raysIn)
                                
{
  TraceRays_ispc *uniform self = (TraceRays_ispc *uniform)_self;
  Lighting_ispc *uniform lights = (Lighting_ispc *uniform)_lights;
  uniform RayList_ispc *uniform raysIn  = (RayList_ispc *uniform)_raysIn;


  uniform float Ka = lights->Ka;

  foreach (i = 0 ... nRaysIn)
  {
    if ((raysIn->type[i] == RAY_PRIMARY) && (raysIn->term[i] & RAY_SURFACE))
    {
      int t = raysIn->term[i];

      float ambient_scale = Ka * (1.0 - raysIn->o[i]);

      raysIn->r[i] += ambient_scale * raysIn->sr[i];
      raysIn->g[i] += ambient_scale * raysIn->sg[i];
      raysIn->b[i] += ambient_scale * raysIn->sb[i];
    }
  }
}

export void TraceRays_generateShadowRays(void *uniform _self,
                                void *uniform uniform _lights,
                                const uniform int nRaysIn,
                                void *uniform uniform _raysIn,
                                uniform int *uniform offsets,
                                void *uniform uniform _raysOut, uniform float global_epsilon)
                                
{
  TraceRays_ispc *uniform self = (TraceRays_ispc *uniform)_self;
  Lighting_ispc *uniform lights = (Lighting_ispc *uniform)_lights;
  uniform RayList_ispc *uniform raysIn  = (RayList_ispc *uniform)_raysIn;
  uniform RayList_ispc *uniform raysOut = (RayList_ispc *uniform)_raysOut;

  uniform float epsilon = global_epsilon;

#ifdef GXY_REVERSE_LIGHTING
  uniform float Kd = -lights->Kd / lights->nLights;
#else
  uniform float Kd = lights->Kd / lights->nLights;
#endif

  foreach (i = 0 ... nRaysIn)
  {
    // Only cast secondary rays for primary rays that hit a surface

    int offset = offsets[i];
    if (offset != -1)
    {
      int t = raysIn->term[i];

      // If a primary ray hit a surface, then do the lighting and cast whatever secondary rays 
      // are necessary. 
      // the block, then propagate it
      
      // Initially, the input ray color is the accumulated result up to the current
      // surface.   If there is, add in the lighted surface color either by casting
      // AO and/or shadow rays, or just light the sucker here.  In any event, the
      // contribution of the surface is attenuated by the current opacity of the primary
      // ray.

      // Going to need the surface normal regardless of whether ambient and diffuse are 
      // added by secondary rays or right here.
        
      vec3f surface_normal = make_vec3f(raysIn->nx[i], raysIn->ny[i], raysIn->nz[i]);

      // If there are shadow rays to be cast, then the Kd of the lighting 
      // model is divided among the cast rays.  Otherwise the Kd * cos(normal, light) times
      // the surface color is added in to the input ray color.  Again, in either case, 
      // the contribution is diminished by the opacity of the goo in front.

      for (int k = 0; k < lights->nLights; k++)
      {
				vec3f surface_point = make_vec3f(raysIn->ox[i] + raysIn->t[i]*raysIn->dx[i] + epsilon*raysIn->nx[i], 
													       		     raysIn->oy[i] + raysIn->t[i]*raysIn->dy[i] + epsilon*raysIn->ny[i],
															           raysIn->oz[i] + raysIn->t[i]*raysIn->dz[i] + epsilon*raysIn->nz[i]);

				vec3f lvec;

				if (lights->types[k])
					lvec = safe_normalize(lights->lights[k] - surface_point);
				else
          lvec = neg(lights->lights[k]);

        lvec = safe_normalize(lvec);
        float d = dot(surface_normal, lvec);

        if (d < 0) d = 0;

        float dff = (1.0 - raysIn->o[i]) * Kd * d;

        float dff_r = dff * raysIn->sr[i];
        float dff_g = dff * raysIn->sg[i];
        float dff_b = dff * raysIn->sb[i];

        raysOut->ox[offset]    = surface_point.x;
        raysOut->oy[offset]    = surface_point.y;
        raysOut->oz[offset]    = surface_point.z;
        raysOut->dx[offset]    = lvec.x;
        raysOut->dy[offset]    = lvec.y;
        raysOut->dz[offset]    = lvec.z;
        raysOut->r[offset]     = dff_r;
        raysOut->g[offset]     = dff_g;
        raysOut->b[offset]     = dff_b;
        raysOut->o[offset]     = 0.0;
        raysOut->t[offset]     = 0.0;
        raysOut->tMax[offset]  = inf;
        raysOut->x[offset]     = raysIn->x[i];
        raysOut->y[offset]     = raysIn->y[i];
        raysOut->type[offset]  = RAY_SHADOW;
        raysOut->term[offset]  = 0;
        offset++;
      }
    }
  }
}

export void TraceRays_diffuseLighting(void *uniform pointer,
                                void *uniform uniform _lights,
                                const uniform int nRaysIn,
                                void *uniform uniform _raysIn)
{
  TraceRays_ispc *uniform self = (TraceRays_ispc *uniform)pointer;
  Lighting_ispc *uniform lights = (Lighting_ispc *uniform)_lights;
  uniform RayList_ispc *uniform raysIn  = (RayList_ispc *uniform)_raysIn;

  uniform float Kd = lights->Kd / lights->nLights;

  foreach (i = 0 ... nRaysIn)
  {
    if ((raysIn->type[i] == RAY_PRIMARY) && (raysIn->term[i] & RAY_SURFACE))
    {
      vec3f surface_normal = make_vec3f(raysIn->nx[i], raysIn->ny[i], raysIn->nz[i]);

      float tot_dff_r = 0;
      float tot_dff_g = 0;
      float tot_dff_b = 0;

      for (int k = 0; k < lights->nLights; k++)
      {
				float d;

        vec3f lvec;
				if (lights->types[k])
				{
					float t = raysIn->t[i];

					vec3f surface_point = make_vec3f(raysIn->ox[i] + t*raysIn->dx[i],
																					 raysIn->oy[i] + t*raysIn->dy[i],
																					 raysIn->oz[i] + t*raysIn->dz[i]);

					lvec = safe_normalize(lights->lights[k] - surface_point);

				}
				else
				{
          lvec = neg(lights->lights[k]);
				}

        d = dot(surface_normal, lvec);

        if (d > 0)
        {
          float dff = (1.0 - raysIn->o[i]) * d;

          float dff_r = dff * raysIn->sr[i];
          float dff_g = dff * raysIn->sg[i];
          float dff_b = dff * raysIn->sb[i];

          tot_dff_r += dff_r;
          tot_dff_g += dff_g;
          tot_dff_b += dff_b;
        }
      }

      raysIn->r[i]     = raysIn->r[i] + Kd * (1 - raysIn->o[i]) * tot_dff_r;
      raysIn->g[i]     = raysIn->g[i] + Kd * (1 - raysIn->o[i]) * tot_dff_g;
      raysIn->b[i]     = raysIn->b[i] + Kd * (1 - raysIn->o[i]) * tot_dff_b;
      raysIn->o[i]     = raysIn->o[i] + Kd * (1 - raysIn->o[i]) * raysIn->o[i];
    }
  }
}
#endif
