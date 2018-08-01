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
#include "ospray/SDK/camera/Camera.ih"
#include "ospray/SDK/common/Model.ih"
#include "ospray/SDK/transferFunction/LinearTransferFunction.ih"
#include "ospray/SDK/volume/Volume.ih"

#include "hits.h"
#include "Lighting.ih"
#include "RayFlags.h"
#include "Rays.ih"
#include "TraceRays.ih"
#include "Visualization.ih"
#include "VolumeVis.ih"

#include "UV.ih"

export void *uniform TraceRays_allocate()
{
  TraceRays_ispc *uniform t = uniform new uniform TraceRays_ispc;
  return (void *)t;
}

export void TraceRays_SetEpsilon(void *uniform _self, uniform float e)
{
  TraceRays_ispc *uniform self = (uniform TraceRays_ispc *)_self;
  self->epsilon = e;
}

export uniform float TraceRays_GetEpsilon(void *uniform _self)
{
  TraceRays_ispc *uniform self = (uniform TraceRays_ispc *)_self;
  return self->epsilon;
}

export void TraceRays_initialize(void *uniform _self)
{
  TraceRays_ispc *uniform self = (uniform TraceRays_ispc *)_self;
  self->epsilon = 0.001;
}

export void TraceRays_destroy(void *uniform ispc)
{
    delete ispc;
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
MyIntersectBox(const Ray& ray,
               const uniform box3f& box,
               float& t0,
               float& t1)
{
  //print("MIB: l\nX %\nY %\nZ %\n", box.lower.x, box.lower.y, box.lower.z); 
  //print("MIB: u\nX %\nY %\nZ %\n", box.upper.x, box.upper.y, box.upper.z); 
  //print("Ray org:\n%\n%\n%\n", ray.org.x, ray.org.y, ray.org.z);
  //print("Ray dir:\n%\n%\n%\n", ray.dir.x, ray.dir.y, ray.dir.z);

  const vec3f mins = (box.lower - ray.org) * rcp(ray.dir);
  const vec3f maxs = (box.upper - ray.org) * rcp(ray.dir);

  t0 = max(min(mins.x,maxs.x),
           max(min(mins.y,maxs.y),
               min(mins.z,maxs.z)));

  t1 = min(max(mins.x,maxs.x),
           min(max(mins.y,maxs.y),
               max(mins.z,maxs.z)));
}

#if 1

inline bool
LookForGeometryHit(varying bool shadeFlag,
                   uniform Model *uniform model,
                   varying Ray &ray,
                   uniform Visualization_ispc *uniform vis,
                   varying Hit &hit)
{
// print("LookForGeometryHit\n");

  traceRay(model, ray);

  if(ray.geomID < 0)
  {
// print("miss!\n");
    return false;
  }
  else
  {
// print("hit!\n");
    if (shadeFlag)
    {
      DifferentialGeometry dg;

      postIntersect(model, dg, ray, DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD|DG_COLOR|DG_MATERIALID|DG_TEXCOORD);

#if 0
      GOING TO HAVE TO MESS AROUND WITH DG TO MAKE GET PER-SPHERE VALUE TO WORK

      hit.color   = make_vec3f(0.6, 0.6, 0.6);
      if (ray.geomID & 1) hit.color.x = 1.0;
      if (ray.geomID & 2) hit.color.y = 1.0;
      if (ray.geomID & 4) hit.color.z = 1.0;

      uniform GeometryVisualization *varying gvis = vis->geometryVisualizations[ray.geomID];

      for (uniform int gid = 0; gid < vis->nGeometryVisualizations; gid++)
        if (ray.geomID == gid)
        {
          uniform GeometryVisualization *uniform gvis = vis->geometryVisualizations[gid];
          uniform TransferFunction *uniform tf = gvis->transferFunction;
          if (tf)
            hit.color = tf->getColorForValue(tf, dg.value);
          else
            print("NO TF\n");
        }
#endif

      hit.color   = make_vec3f(dg.color.x, dg.color.y, dg.color.z);

      hit.opacity = dg.color.w;
      hit.point   = dg.P;
      hit.normal  = dg.Ns;
      hit.t       = ray.t;
    }

    return true;
  }
}

#endif

inline bool
LookForSliceHit(varying bool shadeFlag,
                varying Ray &ray, 
                uniform Visualization_ispc *uniform vis,
                varying Hit &hit)
{
  bool h = false;
  int vid = -1, sid = -1;

  //print("slice tested... nv = %\n", vis->nVolumeVis);
  if (vis->nVolumeVis > 0)
  {
    for (uniform  int major = 0; major < vis->nVolumeVis; major++)      // which volume?
    {
      uniform VolumeVis_ispc *uniform vvis = vis->volumeVis[major];
      //print("nslices: %\n", vvis->nSlices);

      for (uniform int minor = 0; minor < vvis->nSlices; minor ++)
      {
        uniform vec4f plane = vvis->slices[minor];

        //print("plane: % % % %\n", plane.x, plane.y, plane.z, plane.w);

        vec3f pnorm = make_vec3f(plane.x, plane.y, plane.z);

        float d = plane.w;

        float denom = dot(ray.dir, pnorm);

        if (abs(denom) > 0.0001)
        {
          float t = (d - dot(ray.org, pnorm)) / denom;

// print("denom: %\n", denom);
// print("ray t: %\n   t0: %\n", ray.t, ray.t0);
// print("t: %\n", t);

          if (t >= ray.t0 && t <= ray.t)
          {
// print("sh hit % %\n", major, minor);

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
          }
        }
      }
    }
  }

//print("vid %\nsid %\n", vid, sid);
  if (h)
  {

    if (shadeFlag)
    {
      float s[100];
      hit.point = ray.org + hit.t * ray.dir;
 
      for (uniform int major = 0; major <= vid; major++)
        if (major == vid)
        {
          uniform VolumeVis_ispc *uniform vvis = (VolumeVis_ispc *)vis->volumeVis[major];
          uniform TransferFunction *uniform tf = (uniform TransferFunction *uniform )((MappedVis_ispc *)vvis)->transferFunction;
          uniform Volume *uniform vol = (uniform Volume *uniform)(((Vis_ispc *)vvis)->data);

          hit.sample = vol->sample(vol, hit.point);
// print("sample: %\n", hit.sample);
          hit.color = tf->getColorForValue(tf, hit.sample);
// print("color:\nR: %\nG: %\nB: %\n", hit.color.x, hit.color.y, hit.color.z);
          hit.opacity = 1.0;
        }
    }
  }
//else print("sh m\n");
    
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

  //print("Iso: nvol %\n", vis->nVolumeVis);

  for (uniform int major = 0; major < vis->nVolumeVis; major++)      // which volume?
  {
    uniform VolumeVis_ispc *uniform vvis = vis->volumeVis[major];
    uniform Volume *uniform vol = (uniform Volume *uniform)(((Vis_ispc *)vvis)->data);

    float sl = sLast[major];
    float st = sThis[major];

    //print("sl %\nst %\nniso %\n", sl, st, vvis->nIsovalues);

    for (uniform int minor = 0; minor < vvis->nIsovalues; minor++) 
    {
      uniform float isoval = vvis->isovalues[minor];
      //print("% ", isoval);
      if (((isoval >= sl) && (isoval < st)) || ((isoval <= sl) && (isoval > st)))
      {
       // print(" hit\n");

        h          = true;
        vid        = major;
        hit.t      = tLast + ((isoval - sl) / (st - sl)) * (tThis - tLast);
        hit.sample = isoval;
      }
      //else
        //print("miss\n");
    }
  }
  
  if (h)
  {
    hit.point = ray.org + hit.t * ray.dir;

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

// print("S %\nR %\nG %\nB %\n", hit.sample, hit.color.x, hit.color.y, hit.color.z);
          hit.color = tf->getColorForValue(tf, hit.sample);
          hit.opacity = 1.0;
        }
    }

    // hit.color = make_vec3f(0, 0, 1);
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
                               void *uniform _raysIn)
{ 
  uniform TraceRays_ispc *uniform self = (uniform TraceRays_ispc *)_self;
  uniform Visualization_ispc *uniform vis = (uniform Visualization_ispc *)_vis;
  uniform RayList_ispc *uniform raysIn = (uniform RayList_ispc *)_raysIn;
  Model *uniform model = (Model *uniform) vis->model;
  uniform box3f box = vis->local_bb;
  uniform bool integrate = false;
  uniform float step, epsilon;
  uniform Camera *uniform c;

#if 0
  print("BOX [(%f %f %f)-(%f %f %f)]\n",
        box.lower.x,
        box.lower.y,
        box.lower.z,
        box.upper.x,
        box.upper.y,
        box.upper.z);
#endif

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
      epsilon = self->epsilon * s;
    }
  }

// print("nRaysIn: %, integrate = %\n", nRaysIn, integrate);

  foreach (i = 0 ... nRaysIn)
  {
    enum { NONE, TIMEOUT, OPAQUE, SURFACE, BOUNDARY } term = NONE;

    bool shadeFlag = raysIn->type[i] == RAY_PRIMARY;

    Ray ray;

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

    vec4f color = make_vec4f(raysIn->r[i], raysIn->g[i], raysIn->b[i], raysIn->o[i]);
    
    // tExit is the initial termination of the ray - infinity, or the AO radius
    // Its used to determine if loop termination was due to timeout

    varying float tExit  = ray.t;

    // Get the volume enter/exit t's for the ray irrespective of the rays lifetime
    // tExitVolume is the distance to the exit boundary.  Used to determine if loop
    // termination was due to exitting the volume - which MAY terminate the ray or
    // may cause it to be traced into the next partition of a partitioned dataset.
    // The entry can potentially be negative if the ray origin is inside the volume 

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

// print("X %\nY %\n", raysIn->x[i], raysIn->y[i]);
// print("tEn %\ntEx %\n", tEntry, tExitVolume);

    Hit hit;          

    // Look for slice or geometry that stops the ray inside this block.  May continue
    // if its not opaque; but that'll be in a later call to this routine after we've
    // cast the secondary rays

    bool surface_hit = LookForSliceHit(shadeFlag, ray, vis, hit);

    if (model)
      surface_hit |= LookForGeometryHit(shadeFlag, model, ray, vis, hit);

    float tOpaque, tTermination = ray.t;

    if (integrate)
    {
//print("integrating... epsilon %, step %\n", epsilon, step);

      // Now iterate up the interval from tEntry to tTermination, accruing volume contributions and looking for
      // isosurfaces

      varying float tLast, tThis; varying bool opaque = false;
      float sLast[100], sThis[100];   // last and current samples for up to 100 volumes (should be dynamic)

      // Note complicated step - last step to boundary must be included, but step AFTER
      // THAT one must not.

      bool hit_isosurface = false;

      tLast = tEntry + epsilon;

      int iterations = 0;

      for (tThis = tEntry;
           tThis <= tTermination && term != OPAQUE && !hit_isosurface; 
           tThis = (tThis == tEntry) ? (tEntry + epsilon) : (((tThis + step) > tTermination) && (tThis < tTermination)) ? tTermination : tThis + step)
      {
        iterations = iterations + 1;

//print("tThis: %\ntLast: %\n", tThis, tLast);
//print("t %\n", tThis);
        vec3f coord = ray.org + tThis * ray.dir;

        SampleVolumes(coord, vis, sThis);


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
          if (LookForIsoHit(shadeFlag, ray, sLast, sThis, tLast, tThis, vis, hit))
          {
            surface_hit = true;
            tTermination = tThis;
            hit_isosurface = true;

            vec3f coord = ray.org + tThis * ray.dir;
            SampleVolumes(coord, vis, sThis);
          }

          // Go through volumes accumulating opacity
//print("opacity acc loop\n");
          for (uniform int major = 0; major < vis->nVolumeVis; major++)      // which volume?
          {
            uniform VolumeVis_ispc *uniform vvis = vis->volumeVis[major];

            if (vvis->volume_render)
            {
              uniform Volume *uniform vol = (uniform Volume *uniform)((uniform Vis_ispc *uniform)vvis)->data;

              uniform TransferFunction *uniform tf = (uniform TransferFunction *uniform )((MappedVis_ispc *)vvis)->transferFunction;

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

// print("x %\ny %\nz %\ns %\nso %\nwo %\nac %\n", coord.x, coord.y, coord.z, sVolume * 1e11, sampleOpacity, wo, color.w);
// color.x = 0.99, color.y = 0.99; color.z = 0.99;

                }
                else
                {
								  float weightedOpacity = ((tThis - tLast) / step) * clamp(sampleOpacity / vol->samplingRate);
                  color = color * (1-weightedOpacity);
                }
              }
            }
            if (min(min(color.x, color.y), color.z) >= 1.0f || color.w > 0.999f)
            {
              tOpaque = tThis;
              term = OPAQUE;
            } 

//print("acc: %\n", color.w);

            sLast[major] = sThis[major];
          }
//print("opacity acc loop exit\n");

        }

        for (uniform int major = 0; major < vis->nVolumeVis; major++)  
          sLast[major] = sThis[major];

        tLast = tThis;
      }

// print("%\n", iterations);
//if (tThis <= tTermination) print("exit loop T\n");
//if (term == OPAQUE) print("exit loop OPAQUE\n");
//if (hit_isosurface) print("exit loop ISO\n"); 
    
      // To get the above loop to terminate correctly at the termination T, the last valid step is ending T
      // is clamped to the termination T and the subsequent step goes PAST there, though its invalid.
      // So if tThis is beyond the termination T we back it up.

      if (tThis > tTermination) 
        tThis = tTermination;

      raysIn->t[i] = tThis;
      ray.t = tThis;
    }

//print("integration loop exit\n");

    raysIn->r[i] = color.x;
    raysIn->g[i] = color.y;
    raysIn->b[i] = color.z;
    raysIn->o[i] = color.w;

    raysIn->term[i] = 0;
    if (term == OPAQUE)
    {
      // If ray terminated due to VOLUME opacity, set ray current to integration termination

      raysIn->t[i]     = tOpaque;
      raysIn->term[i] |= RAY_OPAQUE;
    }
    else if (surface_hit)
    {
     // Otherwise if it terminated due to a surface hit, set ray current to hit point

      raysIn->t[i]     = hit.t;
      raysIn->term[i] |= RAY_SURFACE;

      if (hit.opacity > 0.999)
        raysIn->term[i] |= RAY_OPAQUE;

      // Save surface info for later shading...

      raysIn->sr[i] = hit.color.x;
      raysIn->sg[i] = hit.color.y;
      raysIn->sb[i] = hit.color.z;
      raysIn->so[i] = 1.0;

      // print("SH\n%\n%\n%\n%\n", hit.color.x, hit.color.y, hit.color.z, 1);

      raysIn->nx[i] = hit.normal.x;
      raysIn->ny[i] = hit.normal.y;
      raysIn->nz[i] = hit.normal.z;

      // print("Surface: Vrgbo\n%\n%\n%\n%\nSrgbo\n%\n%\n%\n%\n", 
          // raysIn->r[i], raysIn->g[i], raysIn->b[i], raysIn->o[i],
          // raysIn->sr[i], raysIn->sg[i], raysIn->sb[i], raysIn->so[i]);

    }
    else if (tTermination == tExitVolume)
    {
      // print("BBBBB\n");
      // May have exitted volume... might have volume rendered, might not...

      raysIn->t[i]     = tExitVolume;
      raysIn->term[i] |= RAY_BOUNDARY;
    }
    else if (tTermination == tTimeout)
    {
      // May have timed out...  

      raysIn->t[i]     = tTimeout;
      raysIn->term[i] |= RAY_TIMEOUT;
    }
    else 
    {
      print("TERMINATION ERROR\n");
    }
  }
}

export void TraceRays_generateAORays(void *uniform _self,
                                void *uniform uniform _lights,
                                const uniform int nRaysIn,
                                void *uniform _raysIn,
                                uniform int *uniform offsets,
                                void *uniform _raysOut)
                                
{
  TraceRays_ispc *uniform self = (TraceRays_ispc *uniform)_self;
  Lighting_ispc *uniform lights = (Lighting_ispc *uniform)_lights;
  uniform RayList_ispc *uniform raysIn  = (RayList_ispc *uniform)_raysIn;
  uniform RayList_ispc *uniform raysOut = (RayList_ispc *uniform)_raysOut;
  uniform float epsilon = self->epsilon;

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
        
#if 0
      print("SN\n%\n%\n%\n", raysIn->nx[i], raysIn->ny[i], raysIn->nz[i]);
#endif

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
                                void *uniform uniform _raysOut)
                                
{
  TraceRays_ispc *uniform self = (TraceRays_ispc *uniform)_self;
  Lighting_ispc *uniform lights = (Lighting_ispc *uniform)_lights;
  uniform RayList_ispc *uniform raysIn  = (RayList_ispc *uniform)_raysIn;
  uniform RayList_ispc *uniform raysOut = (RayList_ispc *uniform)_raysOut;

  uniform float epsilon = self->epsilon;

#ifdef GXY_REVERSE_LIGHTING
  uniform float Kd = -lights->Kd / lights->nLights;
#else
  uniform float Kd = lights->Kd / lights->nLights;
#endif

	// print("KD %\n", Kd);

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

// print("SNX %\nSNY %\nSNZ %\n", surface_normal.x, surface_normal.y, surface_normal.z);

      // If there are shadow rays to be cast, then the Kd of the lighting 
      // model is divided among the cast rays.  Otherwise the Kd * cos(normal, light) times
      // the surface color is added in to the input ray color.  Again, in either case, 
      // the contribution is diminished by the opacity of the goo in front.

      for (int k = 0; k < lights->nLights; k++)
      {
				vec3f surface_point = make_vec3f(raysIn->ox[i] + raysIn->t[i]*raysIn->dx[i] + epsilon*raysIn->nx[i], 
													       		     raysIn->oy[i] + raysIn->t[i]*raysIn->dy[i] + epsilon*raysIn->ny[i],
															           raysIn->oz[i] + raysIn->t[i]*raysIn->dz[i] + epsilon*raysIn->nz[i]);

				// print("ZSP:\nX %\nY %\nZ %\n", surface_point.x, surface_point.y, surface_point.z);

				float d; vec3f lvec;
				if (lights->types[k])
				{
					lvec = safe_normalize(lights->lights[k] - surface_point);
					d = dot(surface_normal, lvec);
			  }
				else
				{
					lvec = safe_normalize(lights->lights[k]);
					d = dot(surface_normal, lights->lights[k]);
				}
					
				// print("LP:\nX %\nY %\nZ %\n", lights->lights[k].x, lights->lights[k].y, lights->lights[k].z);
				// print("LV:\nX %\nY %\nZ %\n", lvec.x, lvec.y, lvec.z);

        if (d < 0) d = 0;

        float dff = (1.0 - raysIn->o[i]) * Kd * d;

        float dff_r = dff * raysIn->sr[i];
        float dff_g = dff * raysIn->sg[i];
        float dff_b = dff * raysIn->sb[i];

				// print("O %\nD %\nDFF %\nR %\nG %\nB %\n", raysIn->o[i], d, dff, dff_r, dff_g, dff_b);

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
// print("Kd: %\n", Kd);

  foreach (i = 0 ... nRaysIn)
  {
    if ((raysIn->type[i] == RAY_PRIMARY) && (raysIn->term[i] & RAY_SURFACE))
    {
      vec3f surface_normal = make_vec3f(raysIn->nx[i], raysIn->ny[i], raysIn->nz[i]);

      //print("SN:\n%\n%\n%\n", surface_normal.x, surface_normal.y, surface_normal.z);

      float tot_dff_r = 0;
      float tot_dff_g = 0;
      float tot_dff_b = 0;

      for (int k = 0; k < lights->nLights; k++)
      {
				float d;

				if (lights->types[k])
				{
					float t = raysIn->t[i];

					vec3f surface_point = make_vec3f(raysIn->ox[i] + t*raysIn->dx[i],
																					 raysIn->oy[i] + t*raysIn->dy[i],
																					 raysIn->oz[i] + t*raysIn->dz[i]);

					vec3f lvec = safe_normalize(lights->lights[k] - surface_point);

#if 0
					print("SP\nX %\ny %\nZ %\n", 
							surface_point.x, surface_point.y, surface_point.z);

					print("SN\nX %\ny %\nZ %\n", 
							surface_normal.x, surface_normal.y, surface_normal.z);

					print("light\nX %\nY %\nZ %\n",
							lights->lights[k].x, lights->lights[k].y, lights->lights[k].z);

					print("LV\nX %\nY %\nZ %\n", lvec.x, lvec.y, lvec.z);
#endif

					d = dot(surface_normal, lvec);
				}
				else
				{
					d = dot(surface_normal, lights->lights[k]);
				}

// print("normal:\n%\n%\n%\n", surface_normal.x, surface_normal.y, surface_normal.z);
// print("light:\n%\n%\n%\n", lights->lights[k].x, lights->lights[k].y, lights->lights[k].z);
// print("d: %\n", d);

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
