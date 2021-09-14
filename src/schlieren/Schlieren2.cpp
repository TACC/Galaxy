/// ========================================================================= //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 //
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

#define _GNU_SOURCE // XXX TODO: what needs this? remove if possible

#include <stdlib.h>
#include "Schlieren2.h"
#include "RayQManager.h"
#include "Particles.h"
#include "Rays.h"
#include "Schlieren2TraceRays.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

#include <vector>

using namespace std;

namespace gxy 
{
WORK_CLASS_TYPE(Schlieren2::NormalizeSchlieren2ImagesMsg);

KEYED_OBJECT_CLASS_TYPE(Schlieren2)

void
Schlieren2::Initialize()
{ 
  RegisterClass();
  NormalizeSchlieren2ImagesMsg::Register();
  super::Initialize();
}

void
Schlieren2::initialize()
{
  super::initialize();
  SetFar(10);
  SetRaysPerPixel(-1);
}

void
Schlieren2::HandleTerminatedRays(RayList *raylist)
{
  RenderingSetP  renderingSet  = raylist->GetTheRenderingSet();
  RenderingP rendering = raylist->GetTheRendering();
  CameraP camera = rendering->GetTheCamera();

  vec3f vp, vd, vu; float aov;

  camera->get_viewpoint(vp);
  camera->get_viewdirection(vd);
  camera->get_viewup(vu);
  camera->get_angle_of_view(aov);

  bool is_ortho = aov == 0.0;

  // Center of image plane

  vec3f center;
  if (is_ortho)
  {
    normalize(vd);
    center = vp + vd * GetFar();
  }
  else
  {
    float d = 1.0 / tan(2*3.1415926*(aov/2.0)/360.0);
    center = vp + (vd * d);
    normalize(vd);
  }

  vec3f vr = cross(vd, vu);
  normalize(vr);

  vu = cross(vr, vd);
  normalize(vu);

  // Get planar equation of projection plane: {vd.x, vd.y, vd.z, d} where d is:

  vec4f projection_plane(vd.x, vd.y, vd.z, -dot(vd, center));

  // Need to be able to transform screen x,y to WCS point

  int width, height;
  rendering->GetTheSize(width, height);

  float pixel_scaling = (((width < height) ? width : height) - 1.0) / 2.0;
  float off_x = ((width - 1) / 2.0);
  float off_y = ((height - 1) / 2.0);

  // for each ray...

  int mx, my;
  float dmax = 0;

  int kkk = 0;
  for (int i = 0; i < raylist->GetRayCount(); i++)
  {
    // If its terminated, project forward to projection plane
    // and determine color by difference
    // between projected destination and original destination

    if (raylist->get_classification(i) == Renderer::TERMINATED)
    {
      // Final point and direction of bent ray

      vec3f p_term(raylist->get_ox(i), raylist->get_oy(i), raylist->get_oz(i));
      vec3f d_term(raylist->get_dx(i), raylist->get_dy(i), raylist->get_dz(i));

      normalize(d_term);

      // We want the intersection of the ray starting at the exit point in
      // the exit direction with the projection plane.  The projection distance
      // is the hyp - the perpendicular distance divided by the cosine of the
      // angle between the perpendicular direction and the exit vector.

      float d_perp = -(dot(projection_plane, p_term) + projection_plane.w);
      float cos_theta = dot(projection_plane, d_term);
      float d_exit_vector = d_perp / cos_theta;
      
      vec3f p_proj = p_term + d_term * d_exit_vector;

      vec3f p_vec = p_proj - center;

      float X = (dot(p_vec, vr) * pixel_scaling) + off_x;
      float Y = (dot(p_vec, vu) * pixel_scaling) + off_y;

      if (X < 0 || X >= width || Y < 0 || Y >= height)
      {
        raylist->set_classification(i, Renderer::DROP_ON_FLOOR);
        continue;
      }

      // Now p_proj is the point the bent ray extends to in the projection plane.   
      // Where would the original ray have hit?

      float pixel_ix = raylist->get_x(i);
      float pixel_iy = raylist->get_y(i);

      float pixel_fx = (pixel_ix - off_x) / pixel_scaling;
      float pixel_fy = (pixel_iy - off_y) / pixel_scaling;

      vec3f pixel_wcs = center + (vr * pixel_fx) + (vu * pixel_fy);

      // distance of pixel_wcs from plane perpendicular to the plane...

      d_perp = -(dot(projection_plane, pixel_wcs) + projection_plane.w);

      vec3f p_proj_orig;
      if (is_ortho)
      {
        // Then project perpendicular to projection plane
        p_proj_orig = pixel_wcs + (vd * d_perp);
      }
      else
      {
        // original ray direction from eye through pixel

        vec3f d_orig = pixel_wcs - vp;
        normalize(d_orig);

        // Distance along original direction depends on angle between original dir and 
        // normal of projection plane

        cos_theta = dot(projection_plane, d_orig);
        float d_orig_vector = d_perp / cos_theta;

        p_proj_orig = pixel_wcs + d_orig * d_orig_vector;
      }

      vec3f delta = p_proj - p_proj_orig;
      if (GetCutoffType() == 0 && delta.x < GetCutoffValue())
      {
        raylist->set_classification(i, Renderer::DROP_ON_FLOOR);
        continue;
      }
      else if (GetCutoffType() == 1 && delta.y < GetCutoffValue())
      {
        raylist->set_classification(i, Renderer::DROP_ON_FLOOR);
        continue;
      }
      else  
      {
        float dd = sqrt(delta.x*delta.x + delta.y*delta.y);
        if (dd < GetCutoffValue())
        {
          raylist->set_classification(i, Renderer::DROP_ON_FLOOR);
          continue;
        }
      }

      raylist->set_r(i, X);
      raylist->set_g(i, Y);
      raylist->set_b(i, 0.0);
      raylist->set_o(i, 0.0);
    }
  }

  std::cerr << kkk << " photons moved bucket\n";

  super::HandleTerminatedRays(raylist);
}

int
Schlieren2::serialSize()
{
  return super::serialSize() + 2*sizeof(float) + 2*sizeof(int);
}

unsigned char *
Schlieren2::serialize(unsigned char *p)
{
  p = super::serialize(p);
  *(int *)p = GetRaysPerPixel();
  p += sizeof(int);
  *(float *)p = GetFar();
  p += sizeof(float);
  *(float *)p = GetCutoffValue();
  p += sizeof(float);
  *(int *)p = GetCutoffType();
  p += sizeof(int);
  return p;
}

unsigned char *
Schlieren2::deserialize(unsigned char *p)
{
  p = super::deserialize(p);
  SetRaysPerPixel(*(int *)p);
  p += sizeof(int);
  SetFar(*(float *)p);
  p += sizeof(float);
  SetCutoffValue(*(float *)p);
  p += sizeof(float);
  SetCutoffType(*(int *)p);
  p += sizeof(int);
  return p;
}

void
Schlieren2::Trace(RayList *raylist)
{
  RendererP      renderer  = raylist->GetTheRenderer();
  RenderingSetP  renderingSet  = raylist->GetTheRenderingSet();
  RenderingP     rendering     = raylist->GetTheRendering();
  VisualizationP visualization = rendering->GetTheVisualization();

  // This is called when a list of rays is pulled off the
  // RayQ.  When we are done with it we decrement the
  // ray list count (rather than when it was pulled off the
  // RayQ) so we don't send a message upstream saying we are idle
  // until we actually are.

  Schlieren2TraceRays tracer;

  RayList *out = tracer.Trace(rendering->GetLighting(), visualization, raylist);
  if (out)
  {
    if (out->GetRayCount() > renderer->GetMaxRayListSize())
    {
      vector<RayList*> rayLists;
      out->Split(rayLists);
      for (vector<RayList*>::iterator it = rayLists.begin(); it != rayLists.end(); it++)
      {
        RayList *s = *it;
        renderingSet->Enqueue(*it);
      }
      delete out;
    }
    else
      renderingSet->Enqueue(out);
  }
}

bool
Schlieren2::LoadStateFromValue(Value& v)
{
  if (v.HasMember("far"))
      SetFar(v["far"].GetDouble());

  if (v.HasMember("rays per pixel"))
      SetRaysPerPixel(v["rays per pixel"].GetInt());

  return true;
}

void
Schlieren2::SaveStateToValue(Value& v, Document& doc)
{
  v.AddMember("far", Value().SetDouble(GetFar()), doc.GetAllocator());
}

bool
Schlieren2::NormalizeSchlieren2ImagesMsg::CollectiveAction(MPI_Comm c, bool is_root)
{
  return false;

  char *ptr = (char *)contents->get();
  Key key = *(Key *)ptr;
  RenderingSetP rs = RenderingSet::GetByKey(key);

  for (int i = 0; i < rs->GetNumberOfRenderings(); i++)
  {
    RenderingP r = rs->GetRendering(i);
    if (r->IsLocal())
    {
      int width, height;
      r->GetTheSize(width, height);

      int n = width * height;

      float *p = r->GetPixels();

      float mm[3] = {p[0], p[1], p[2]};
      float MM[3] = {p[0], p[1], p[2]};
      for (int i = 0; i < n; i++, p += 4)
      {
        for (int j = 0; j < 3; j++)
        {
          if (p[j] < mm[j]) mm[j] = p[j];
          if (p[j] > MM[j]) MM[j] = p[j];
        }
      }

      std::cerr << "r " << mm[0] << " " << MM[0] << "\n";
      std::cerr << "g " << mm[1] << " " << MM[1] << "\n";
      std::cerr << "b " << mm[2] << " " << MM[2] << "\n";

      p = r->GetPixels();
      for (int i = 0; i < n; i++, p += 4)
        for (int j = 0; j < 3; j++)
          if (MM[j] != mm[j])
            p[j] = (p[j] - mm[j])/(MM[j] - mm[j]);
    }
  }

  return false;
}

void
Schlieren2::NormalizeImages(RenderingSetP rs)
{
  NormalizeSchlieren2ImagesMsg msg(rs);
  msg.Broadcast(true, true);
}

void
Schlieren2::local_render(RendererP renderer, RenderingSetP renderingSet)
{
  // NeedInitialRays tells us whether we need to generate initial rays
  // for the current frame.  It is possible that this RenderingSet has
  // already seen a raylist from a later frame, in which case we won't
  // bother generating rays for this one.
  
  int fnum = renderingSet->NeedInitialRays();
  if (fnum != -1)
  {
#ifdef GXY_WRITE_IMAGES
    GetTheRayQManager()->Pause();

    if (renderingSet->CameraIsActive())
    {
      std::cerr << "RenderingSet: Cannot InitializeState when camera is already active\n";
      exit(0);
    }
  
    // Bump to keep from seeming finished until all camera rays have been spawned

    renderingSet->IncrementActiveCameraCount();

    GetTheRayQManager()->Resume();
#endif

#ifdef GXY_WRITE_IMAGES
    renderingSet->initializeSpawnedRayCount();
#endif

    vector<future<int>> rvec;
    for (int i = 0; i < renderingSet->GetNumberOfRenderings(); i++)
    {
      RenderingP rendering = renderingSet->GetRendering(i);
      rendering->resolve_lights(renderer);

      CameraP camera = rendering->GetTheCamera();
      VisualizationP visualization = rendering->GetTheVisualization();

      Box *gBox = visualization->get_global_box();
      Box *lBox = visualization->get_local_box();

#if 0
      if (GetRaysPerPixel() == -1)
        camera->generate_initial_rays(renderer, renderingSet, rendering, lBox, gBox, rvec, fnum);
      else
        for (int i = 0; i < GetRaysPerPixel(); i++)
          camera->generate_initial_rays(renderer, renderingSet, rendering, lBox, gBox, rvec, fnum, i);
#else
      camera->generate_initial_rays(renderer, renderingSet, rendering, lBox, gBox, rvec, fnum);
#endif
    }

#ifdef GXY_PRODUCE_STATUS_MESSAGES
    renderingSet->_dumpState(c, "status"); // Note this will sync after cameras, I think
#endif

#ifdef GXY_EVENT_TRACKING
    GetTheEventTracker()->Add(new CameraLoopEndEvent);
#endif

#ifdef GXY_WRITE_IMAGES
    for (auto& r : rvec)
      r.get();

    renderingSet->DecrementActiveCameraCount(0);
#endif // GXY_WRITE_IMAGES
  }
  
}

} // namespace gxy
