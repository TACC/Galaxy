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
KEYED_OBJECT_CLASS_TYPE(Schlieren)

void
Schlieren::Initialize()
{ 
  RegisterClass();
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

  vec3f vr = cross(vd, vu);
  normalize(vr);

  vu = cross(vr, vd);
  normalize(vu);

  normalize(vd);
  vec3f C = vp + vd*GetFar();   // Center of image plane in 3D - choose far to put the plane *beyond* the data

  // Get planar equation of projection plane: {vd.x, vd.y, vd.z, d} where d is:

  vec4f projection_plane(vd.x, vd.y, vd.z, -dot(vd, C));
  std::cerr << "p_plane: " << projection_plane.x << " " << projection_plane.y << " " << projection_plane.z << " " << projection_plane.w << "\n";

  // Need to be able to transform screen x,y to WCS point

  int width, height;
  rendering->GetTheSize(width, height);

  // These are factors used to convert pixel locations to WCS points.  These'll
  // be used to determined where the *original* ray *would have* hit the projection
  // plane.

  float off_x = ((width - 1) / 2.0);
  float off_y = ((height - 1) / 2.0);
  float pixel_scaling = (((width < height) ? width : height) - 1.0) / 2.0;
  float d = 1.0 / tan(2*3.1415926*(aov/2.0)/360.0);
  vec3f center = vec3f(vp) + (vd * d);

  // for each ray...

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
      // angle between the perpendicular direction and the 
      // point projected along the exitting direction to the projection plane.
      // The length of projection will be the perpendicular distance
      // 

      

      // distance of p_term from plane perpendicular to the plane...

      float d_perp = -(dot(projection_plane, p_term) + projection_plane.w);

      // theta is the angle between the *exitting* ray and the perpendicular

      float cos_theta = dot(projection_plane, d_term);
      
      std::cerr << "p_term: " << p_term.x << " " << p_term.y << " " << p_term.z << "\n";
      std::cerr << "d_term: " << d_term.x << " " << d_term.y << " " << d_term.z << "\n";
      std::cerr << "d_perp: " << d_perp << "\n";

      vec3f p_proj = p_term + d_term*d_perp;

      std::cerr << "p_proj: " << p_proj.x << " " << p_proj.y << " " << p_proj.z << "\n";

      // Now p_proj is the point the bent ray extends to in the projection plane.   
      // Where would the original ray have hit?

      int pixel_ix = raylist->get_x(i);
      int pixel_iy = raylist->get_y(i);

      float pixel_fx = (pixel_ix - off_x) / pixel_scaling;
      float pixel_fy = (pixel_iy - off_y) / pixel_scaling;

      vec3f pixel_wcs = center + (vr * pixel_fx) + (vu * pixel_fy);

      // distance of pixel_wcs from plane perpendicular to the plane...

      d_perp = -(dot(projection_plane, pixel_wcs) + projection_plane.w);

      // original direction

      vec3f d_orig = pixel_wcs - vp;
      normalize(d_orig);

      vec3f p_proj_orig = pixel_wcs + d_orig * d_perp;
      std::cerr << "orig p_proj: " << p_proj_orig.x << " " << p_proj_orig.y << " " << p_proj_orig.z << "\n";
    }
  }

  super::HandleTerminatedRays(raylist);
}

int
Schlieren::SerialSize()
{
  return super::SerialSize() + sizeof(float);
}

unsigned char *
Schlieren::Serialize(unsigned char *p)
{
  p = super::Serialize(p);
  *(float *)p = GetFar();
  p += sizeof(float);
  return p;
}

unsigned char *
Schlieren::Deserialize(unsigned char *p)
{
  p = super::Deserialize(p);
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

  SchlierenTraceRays tracer;

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

} // namespace gxy
