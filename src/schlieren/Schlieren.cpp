// ========================================================================== //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 k/
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
#include "Schlieren.h"
#include "Particles.h"
#include "Rays.h"
#include "SchlierenTraceRays.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

#include <vector>

using namespace std;

namespace gxy 
{
WORK_CLASS_TYPE(Schlieren::NormalizeSchlierenImagesMsg);

KEYED_OBJECT_CLASS_TYPE(Schlieren)

void
Schlieren::Initialize()
{ 
  RegisterClass();
  NormalizeSchlierenImagesMsg::Register();
  super::Initialize();
}

void
Schlieren::HandleTerminatedRays(RayList *raylist)
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

  // These are factors used to convert pixel locations to WCS points.  These'll
  // be used to determined where the *original* ray *would have* hit the projection
  // plane.

  float off_x = ((width - 1) / 2.0);
  float off_y = ((height - 1) / 2.0);
  float pixel_scaling = (((width < height) ? width : height) - 1.0) / 2.0;

  // for each ray...

  int mx, my;
  float dmax = 0;

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

      if (raylist->get_x(i) == 1 && raylist->get_y(i) == 6)
      {
        std::cerr << "o " << raylist->get_ox(i) << " " << raylist->get_oy(i) << " " << raylist->get_oz(i) << "\n";
        std::cerr << "d " << raylist->get_dx(i) << " " << raylist->get_dy(i) << " " << raylist->get_dz(i) << "\n";
      }

      normalize(d_term);

      // We want the intersection of the ray starting at the exit point in
      // the exit direction with the projection plane.  The projection distance
      // is the hyp - the perpendicular distance divided by the cosine of the
      // angle between the perpendicular direction and the exit vector.

      float d_perp = -(dot(projection_plane, p_term) + projection_plane.w);
      float cos_theta = dot(projection_plane, d_term);
      float d_exit_vector = d_perp / cos_theta;
      
      vec3f p_proj = p_term + d_term * d_exit_vector;

      // Now p_proj is the point the bent ray extends to in the projection plane.   
      // Where would the original ray have hit?

      int pixel_ix = raylist->get_x(i);
      int pixel_iy = raylist->get_y(i);

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
      
      float dd = sqrt(delta.x*delta.x + delta.y*delta.y + delta.z*delta.z);
      if (dd > dmax)
      {
        mx = pixel_ix;
        my = pixel_iy;
        dmax = dd;
      }

      raylist->set_r(i, delta.x);
      raylist->set_g(i, delta.y);
      raylist->set_b(i, delta.z);
      raylist->set_o(i, dd);
    }
  }

  super::HandleTerminatedRays(raylist);
}

int
Schlieren::serialSize()
{
  return super::serialSize() + sizeof(float);
}

unsigned char *
Schlieren::serialize(unsigned char *p)
{
  p = super::serialize(p);
  *(float *)p = GetFar();
  p += sizeof(float);
  return p;
}

unsigned char *
Schlieren::deserialize(unsigned char *p)
{
  p = super::deserialize(p);
  SetFar(*(float *)p);
  p += sizeof(float);
  return p;
}

void
Schlieren::Trace(RayList *raylist)
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

  SchlierenTraceRays tracer(GetPartitioning());

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
Schlieren::LoadStateFromValue(Value& v)
{
  if (v.HasMember("far"))
      SetFar(v["far"].GetDouble());

  return true;
}

void
Schlieren::SaveStateToValue(Value& v, Document& doc)
{
  v.AddMember("far", Value().SetDouble(GetFar()), doc.GetAllocator());
}

bool
Schlieren::NormalizeSchlierenImagesMsg::CollectiveAction(MPI_Comm c, bool is_root)
{
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


      p = r->GetPixels();
      float m[4] = {0.0, 0.0, 0.0};
      for (int i = 0; i < n; i++, p += 4)
        for (int j = 0; j < 4; j++)
          if (!i || (abs(p[j]) > m[j]))
            m[j] = abs(p[j]);

      p = r->GetPixels();
      for (int i = 0; i < n; i++, p += 4)
      {
        for (int j = 0; j < 3; j++)
          if (m[j])
            p[j] = 0.5 + 0.5*(p[j] / m[j]);
          else 
            p[j] = 0.5;

        if (m[3])
          p[3] = p[3] / m[3];
        else 
          p[3] = 0.5;
      }
    }
  }

  return false;
}

void
Schlieren::NormalizeImages(RenderingSetP rs)
{
  NormalizeSchlierenImagesMsg msg(rs);
  msg.Broadcast(true, true);
}

} // namespace gxy
