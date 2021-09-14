// ========================================================================== //
// Copyright (c) 2014-2020 The University of Texas at Austin.                 //
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

#define FUZZ 0.000001
static float fuzz = FUZZ;

#include "Camera.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <float.h>
#include <iostream>
#include <math.h>
#include <sstream>
#include <string.h>
#include <string>
#include <tbb/tbb.h>
#include <vector>

#include "Application.h"
#include "MessageManager.h"
#include "RayFlags.h"
#include "Rays.h"
#include "Renderer.h"
#include "Rendering.h"
#include "RenderingSet.h"
#include "Threading.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

using namespace rapidjson;
using namespace std;

namespace pt = boost::property_tree;

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(Camera)

bool full_window;
bool raydebug;
static int Xmax, Xmin, Ymax, Ymin;

void
generate_permutation(vector<int>& p, int num)
{
  std::srand (unsigned(std::time(0)));
  p.clear();
  for (int i=0; i<num; i++) p.push_back(i);
  std::random_shuffle (p.begin(), p.end() );
}

void
Camera::Register()
{
  RegisterClass();
}

bool
Camera::LoadCamerasFromJSON(Value& v, vector<CameraP>& cameras)
{
  if (v.HasMember("Camera") || v.HasMember("Cameras"))
  {
    Value& c = v.HasMember("Camera") ? v["Camera"] : v["Cameras"];
    if (c.IsArray())
    {
      for (int i = 0; i < c.Size(); i++)
      {
        CameraP cam = Camera::NewP();
        if (! cam->LoadFromJSON(c[i]))
        {
          std::cerr << "unable to load " << i << "th camera from JSON\n";
          return false;
        }
        else
          cameras.push_back(cam);
      }
    }
    else
    {
      CameraP cam = Camera::NewP();
      if (! cam->LoadFromJSON(c))
      {
        std::cerr << "unable to load camera from JSON\n";
        return false;
      }
      else
        cameras.push_back(cam);
    }
  }
  else
    std::cerr << "No cameras found\n";

  return true;
}

bool 
Camera::LoadFromPVCC(const char *filename)
{
  float center[3];
  pt::ptree tree;

  try {
    read_xml(filename, tree);

    pt::ptree proxy = tree.get_child("PVCameraConfiguration.Proxy");
    BOOST_FOREACH(pt::ptree::value_type& v, proxy)
    {
      if (v.first == "Property")
      {
        float values[] = { -1e32, -1e32, -1e32 };
        std::string propertyName = v.second.get<std::string>("<xmlattr>.name");
        BOOST_FOREACH(pt::ptree::value_type& vv, v.second)
        {
          if (vv.first == "Element")
          {
            int indx = vv.second.get<int>("<xmlattr>.index");
            float val = vv.second.get<float>("<xmlattr>.value");
            values[indx] = val;
          }
        }
        if (propertyName == "CameraPosition")
          for (int i = 0; i < 3; i++)
            eye[i] = values[i];
        else if (propertyName == "CameraFocalPoint")
          for (int i = 0; i < 3; i++)
            center[i] = values[i];
        else if (propertyName == "CameraViewUp")
          for (int i = 0; i < 3; i++)
            up[i] = values[i];
        else if (propertyName == "CameraViewAngle")
          aov = values[0];
      }
    }
  }
  catch(...) { return false; }

  dir[0] = center[0] - eye[0];
  dir[1] = center[1] - eye[1];
  dir[2] = center[2] - eye[2];
    
  return true;
}

bool 
Camera::LoadFromJSON(Value& v)
{
  if (v.IsString())
  {
    ifstream ifs(v.GetString());
    if (!ifs)
    {
      std::cerr << "unable to open " << v.GetString() << "\n";
      set_error(1);
      return false;
    }

    stringstream ss;
    ss << ifs.rdbuf();

    Document doc;
    if (doc.Parse<0>(ss.str().c_str()).HasParseError())
    {
      if (! LoadFromPVCC(v.GetString()))
      {
        std::cerr << "error loading camera from " << v.GetString() << "\n";
        set_error(1);
        return false;
      }
      else return true;
    }

    float center[3];

    if (! doc.HasMember("PVCameraConfiguration") || 
        !doc["PVCameraConfiguration"].HasMember("Proxy") || 
        !doc["PVCameraConfiguration"]["Proxy"].HasMember("Property"))
    {
      std::cerr << "invalid Paraview camera file: " << v.GetString() << "\n";
      return false;
    }
    
    Value& properties = doc["PVCameraConfiguration"]["Proxy"]["Property"];
    for (int i = 0; i < properties.Size(); i++)
    {
      Value& p = properties[i];

      if (p.HasMember("@name"))
      {
        string name = p["@name"].GetString();
        if (name == "CameraPosition")
        {
          eye[0] = atof(p["Element"][0]["@value"].GetString());
          eye[1] = atof(p["Element"][1]["@value"].GetString());
          eye[2] = atof(p["Element"][2]["@value"].GetString());
        }
        else if (name == "CameraFocalPoint")
        {
          center[0] = atof(p["Element"][0]["@value"].GetString());
          center[1] = atof(p["Element"][1]["@value"].GetString());
          center[2] = atof(p["Element"][2]["@value"].GetString());
        }
        else if (name == "CameraViewUp")
        {
          up[0] = atof(p["Element"][0]["@value"].GetString());
          up[1] = atof(p["Element"][1]["@value"].GetString());
          up[2] = atof(p["Element"][2]["@value"].GetString());
        }
        else if (name == "CameraViewAngle")
          aov = atof(p["Element"]["@value"].GetString());
      }
    }
    dir[0] = center[0] - eye[0];
    dir[1] = center[1] - eye[1];
    dir[2] = center[2] - eye[2];
  }
  else
  {
    if (v.HasMember("annotation"))
    {
      SetAnnotation(string(v["annotation"].GetString()));
    }

    eye[0] = v["viewpoint"][0].GetDouble();
    eye[1] = v["viewpoint"][1].GetDouble();
    eye[2] = v["viewpoint"][2].GetDouble();

    if (v.HasMember("viewdirection"))
    {
      dir[0] = v["viewdirection"][0].GetDouble();
      dir[1] = v["viewdirection"][1].GetDouble();
      dir[2] = v["viewdirection"][2].GetDouble();
    }
    else if (v.HasMember("viewcenter"))
    {
      dir[0] = v["viewcenter"][0].GetDouble() - eye[0];
      dir[1] = v["viewcenter"][1].GetDouble() - eye[1];
      dir[2] = v["viewcenter"][2].GetDouble() - eye[2];
    }
    else
    {
      std::cerr << "need either viewdirection or viewcenter\n";
      set_error(1);
      return false;
    }

    if (v.HasMember("dimensions"))
    {
      set_width(v["dimensions"][0].GetInt());
      set_height(v["dimensions"][0].GetInt());
    }

    up[0] = v["viewup"][0].GetDouble();
    up[1] = v["viewup"][1].GetDouble();
    up[2] = v["viewup"][2].GetDouble();

    aov = v["aov"].GetDouble();
  }

  return true;
}

void
Camera::set_viewdirection(float x, float y, float z)
{
  float d = 1.0 / sqrt(x*x + y*y + z*z);
  dir[0] = x * d;
  dir[1] = y * d;
  dir[2] = z * d;
}

void
Camera::set_viewup(float x, float y, float z)
{
  float d = 1.0 / sqrt(x*x + y*y + z*z);
  up[0] = x * d;
  up[1] = y * d;
  up[2] = z * d;
}

static bool
intersect_line_plane(vec3f& point_on_line, vec3f& line, vec3f& plane_abc, float plane_d, vec3f& intersection_point)
{
  //  ((point_on_line + t*line) * plane_abc) + plane_d = 0    solve for t
  //  
  //  t = -((point_on_line * plane_abc) + plane_d) / (plane_abc * L)
  //  intersection_point = point_on_line + t*line

  float denom = plane_abc * line;     // is the line perpendicular to the plane normal?  If so, it'll never hit
  if (denom < 1e-6) 
    return false;

  intersection_point = point_on_line - (line * (((point_on_line  * plane_abc) + plane_d) / denom));
  return true;
}

class InitialRaysEvent : public Event
{
public:
  InitialRaysEvent(RayList *rayList)
  {
    count = rayList->GetRayCount();
    rset = rayList->GetTheRenderingSet()->getkey();
  }

protected:
  void print(ostream& o)
  {
    Event::print(o);
    o << "Camera rays: " << count << " for rset " << rset;
  }

private:
  int count;
  Key rset;
};

class CameraTaskStartEvent : public Event
{
public:
  CameraTaskStartEvent() {}

protected:
  void print(ostream& o)
  {
    Event::print(o);
    o << "CameraTask start";
  }
};

class CameraTaskEndEvent : public Event
{
public:
  CameraTaskEndEvent() {}

protected:
  void print(ostream& o)
  {
    Event::print(o);
    o << "CameraTask end";
  }
};

// Given a slew of camera parameters and a range of pixels that MIGHT
// be first-hit, figure out which actually are, create a raylist for them,
// and queue them for processing

// NOTE: spawning rays is the highest priority task

int
Camera::spawn_rays_task::work()
{
  if (! a->rs->IsActive(a->fnum))
    return 0;

  return a->camera->SpawnRays(a, start, count);
}

bool 
Camera::SpawnRays(std::shared_ptr<spawn_rays_args> a, int start, int count)
{
#if defined(GXY_EVENT_TRACKING)
  GetTheEventTracker()->Add(new CameraTaskStartEvent());
#endif
    
  RayList *rlist = NULL;

  bool is_ortho = a->camera->get_angle_of_view() == 0.0;

  vec3f vdir, veye;
  a->camera->get_viewdirection(vdir);
  a->camera->get_viewpoint(veye);

  int dst = 0;
  for (int i = 0; i < count; i++)
  {
    int pindex = i + start;
    int x, y;
    vec3f xy_wcs, vray, vorigin;

    // Figure out the pixel (x,y) for the pindex'th pixel

    int p = a->camera->permute ? a->camera->permutation[pindex] : pindex;

    x = a->ixmin + (p % a->iwidth);
    y = a->iymin + (p / a->iwidth);

    // Get pixel location in (-1,1) space

    float fx = (x - a->off_x) * a->scaling;
    float fy = (y - a->off_y) * a->scaling;

    // Get the pixel location in 3D camera space 

    xy_wcs.x = a->center.x + fx * a->vr.x + fy * a->vu.x;
    xy_wcs.y = a->center.y + fx * a->vr.y + fy * a->vu.y;
    xy_wcs.z = a->center.z + fx * a->vr.z + fy * a->vu.z;

    if (is_ortho)
    {
      vorigin = xy_wcs - vdir;
      vray = vdir;
    }
    else
    {
      vorigin = a->veye;
      vray = xy_wcs - a->veye;
      normalize(vray);
    }

    float gmin, gmax;
    bool hit = a->gbox->intersect(vorigin, vray, gmin, gmax);

    float lmin, lmax;
    if (hit) 
    {
      hit = a->lbox->intersect(vorigin, vray, lmin, lmax);
    }

    float d = fabs(lmin) - fabs(gmin);
    if (hit && (lmax >= 0) && (d < fuzz) && (d > -fuzz))
    {
      if (!rlist) rlist = new RayList(a->renderer, a->rs, a->r, count, a->fnum, RayList::PRIMARY);

      rlist->set_x(dst, x);
      rlist->set_y(dst, y);
      rlist->set_ox(dst, vorigin.x);
      rlist->set_oy(dst, vorigin.y);
      rlist->set_oz(dst, vorigin.z);
      rlist->set_dx(dst, vray.x);
      rlist->set_dy(dst, vray.y);
      rlist->set_dz(dst, vray.z);
      rlist->set_r(dst, 0.0);
      rlist->set_g(dst, 0.0);
      rlist->set_b(dst, 0.0);
      rlist->set_o(dst, 0);
      rlist->set_t(dst, 0);
      rlist->set_tMax(dst, FLT_MAX);
      rlist->set_type(dst, RAY_PRIMARY);
      dst++;
    }
  }

  if (rlist)
  {
    if (a->rs->IsActive(a->fnum))
    {
      if (dst < rlist->GetRayCount())
        rlist->Truncate(dst);

#ifdef GXY_EVENT_TRACKING
      GetTheEventTracker()->Add(new InitialRaysEvent(rlist));
#endif

      a->renderer->add_originated_ray_count(rlist->GetRayCount());
      a->rs->Enqueue(rlist, true);
    }
    else
    {
      delete rlist;
    }
  }

#ifdef GXY_WRITE_IMAGES
  a->rs->DecrementActiveCameraCount(dst);        
#endif

#ifdef GXY_EVENT_TRACKING
  GetTheEventTracker()->Add(new CameraTaskEndEvent());
#endif

  return 0;
}

void check_env(RendererP renderer, int width, int height)
{
  static bool first = true;

  if (first)
  {
    first = false;

    full_window =  getenv("GXY_FULLWINDOW") != NULL;
    raydebug    =  getenv("GXY_RAYDEBUG") != NULL;

    if (getenv("GXY_FUZZ"))
      fuzz = atof(getenv("GXY_FUZZ"));

    if (getenv("GXY_X"))
      Xmin = Xmax = atoi(getenv("GXY_X"));
    else
    {
      Xmin = getenv("GXY_XMIN") ? atoi(getenv("GXY_XMIN")) : (width / 2) - 2;
      Xmax = getenv("GXY_XMAX") ? atoi(getenv("GXY_XMAX")) : (width / 2) + 2;
      if (Xmin < 0) Xmin = 0;
      if (Xmax >= (width-1)) Xmax = width-1;
    }
    
    if (getenv("GXY_Y"))
      Ymin = Ymax = atoi(getenv("GXY_Y"));
    else
    {
      Ymin = getenv("GXY_YMIN") ? atoi(getenv("GXY_YMIN")) : (height / 2) - 2;
      Ymax = getenv("GXY_YMAX") ? atoi(getenv("GXY_YMAX")) : (height / 2) + 2;
      if (Ymin < 0) Ymin = 0;
      if (Ymax >= (height-1)) Ymax = height-1;
    }
  }
}
    
void
Camera::generate_initial_rays(RendererP renderer, RenderingSetP renderingSet, RenderingP rendering, Box* lbox, Box *gbox, std::vector<std::future<int>>& rvec, int fnum)
{
  int rays_per_packet = renderer->GetMaxRayListSize();

  int width, height;
  rendering->GetTheSize(width, height);

  check_env(renderer, width, height);

  permute = renderer->GetPermutePixels();
  if (permute && permutation.size() != width*height)
    generate_permutation(permutation, width*height);

  vec3f veye(eye);
  vec3f vu(up);
  vec3f vdir(dir);

  // Center is the center of the image plane in WCS

  vec3f center;
  if (aov == 0.0) // that is, orthographic...
  {
    center = veye + vdir;
    normalize(vdir);
  }
  else
  {
    float d = 1.0 / tan(2*3.1415926*(aov/2.0)/360.0);
    normalize(vdir);
    center = veye + (vdir * d);
  }

  // Right is the normalized cross product of the camera view direction
  // and the up direction (which better not be colinear)

  vec3f vr = cross(vdir, vu);
  normalize(vr);

  // Actual up direction is the normalized cross product of the camera 
  // view direction and the right direction.

  vu = cross(vr, vdir);
  normalize(vu);

  // Want to fit the (-1,-1) -> (1,1) box into the aspect ratio, so we find
  // the min of width and height and scale by (that-1) / 2

  float pixel_scaling = (((width < height) ? width : height) - 1.0) / 2.0;

  // To go to lower-left coordinates, we need to subtract off ((width - 1) / 2.0) and 
  // ((height - 1) / 2.0)

  float off_x = ((width - 1) / 2.0);
  float off_y = ((height - 1) / 2.0);

  // We'll only generate rays for pixels that intersect the local box.   If we are 
  // inside the box, that's all of them.   Otherwise, project the box onto the screen

  int ixmin = 0;
  int iymin = 0;
  int ixmax = width - 1;
  int iymax = height - 1;

  if (! lbox->isIn(veye))
  {
    // Figure out the bb of the projection of the box onto the screen
  
    float w = -(center * vdir);        // the image plane is (vdir, x)

    float minx = 1e6, maxx = -1e6;
    float miny = 1e6, maxy = -1e6;

    // Go through the corners of the box, project into the image plane and then
    // to pixel coordinates

    for (int i = 0; i < 8; i++)
    {
      vec3f proj, corner = lbox->corner(i);
  
      // This gives the projection of the corner onto the image plane in WCS

      if (aov == 0.0) // orthographic
        intersect_line_plane(corner, vdir, vdir, w, proj);
      else
      {
        vec3f line = corner - veye;
        intersect_line_plane(corner, line, vdir, w, proj);
      }
    
      // This gives the vector from the projection to the center of the image in WCS
      proj = proj - center;

      // APP_LOG(<< "corner " << corner.x << " " << corner.y << " " << corner.z << " proj " << proj.x << " " << proj.y << " " << proj.z);

      // Rotate the projection point around the Z axis - now in coordinates of
      // (vr, vu, vdir) but we don't care about Z - the points all lie in the image
      // plane Z=vdist

      float x = proj * vr;
      float y = proj * vu;

      if (x < minx) minx = x;
      if (x > maxx) maxx = x;

      if (y < miny) miny = y;
      if (y > maxy) maxy = y;
    }

    // Now we have the projected BB in (vr, vu) coordinates centered at the center
    // of the image plane

    // We scale the BB and round down on the low side and up on the high side
    // and clip to the screen

    int ixmin = (minx * pixel_scaling) + off_x;
    if (ixmin < 0) ixmin = 0;

    int ixmax = ((maxx * pixel_scaling) + 1) + off_x;
    if (ixmax >= width) ixmax = width - 1;

    int iymin = (miny * pixel_scaling) + off_y;
    if (iymin < 0) iymin = 0;

    int iymax = ((maxy * pixel_scaling) + 1) + off_y;
    if (iymax >= height) iymax = height - 1;
  }
  
  if (full_window)
  {
    ixmin = iymin = 0;
    ixmax = width - 1;
    iymax = height - 1;
  }

  // Given this math, the steps to go from a screen pixel to a WCS point are:
  // 1.  The screen pixels are relative to the LL - need to center
  //      x = x - off_x
  //      y = y - off_y
  // 2.  Scale to WCS units rather than pixels
  //      x = x / pixel_scaling
  //      y = y / pixel_scaling
  // 2.  Un-rotate 
  //      x = x * vr
  //      y = y * vu
  
  if (raydebug)
  {
    if ((Xmin > ixmax) || (Xmax < ixmin) || (Ymin > iymax) || (Ymax < iymin))
      return;

    if (Xmin < ixmin) Xmin = ixmin;
    if (Xmax > ixmax) Xmax = ixmax;
    if (Ymin < iymin) Ymin = iymin;
    if (Ymax > iymax) Ymax = iymax;

    int nrays = ((Ymax-Ymin) + 1) * ((Xmax-Xmin) + 1);
    if (nrays < 0)
      return;

    RayList *rayList = new RayList(renderer, renderingSet, rendering, nrays, fnum, RayList::PRIMARY);

    vec2i *test_rays = new vec2i[nrays];

    int kk = 0;
    for (int y = 0; y < (Ymax-Ymin) + 1; y++)
    {
      for (int x = 0; x < (Xmax-Xmin) + 1; x++)
      {
        rayList->set_x(kk, Xmin + x);
        rayList->set_y(kk, Ymin + y);
        kk++;
      }
    }
    
    kk = 0;
    int k;
    for (k = 0; k < nrays; k++)
    {
      int x = rayList->get_x(k);
      int y = rayList->get_y(k);

      vec3f xy_wcs, vray, vorigin = veye;
      float fx = (x - off_x) / pixel_scaling;
      float fy = (y - off_y) / pixel_scaling;

      xy_wcs.x = center.x + fx * vr.x + fy * vu.x;
      xy_wcs.y = center.y + fx * vr.y + fy * vu.y;
      xy_wcs.z = center.z + fx * vr.z + fy * vu.z;

      if (aov == 0) // orthographic...
      { 
        vorigin = xy_wcs - vdir;
        vray = vdir;
      }
      else
      {
        vorigin = veye;
        vray = xy_wcs - veye;
        normalize(vray);
      }

      float gmin, gmax;
      bool hit = gbox->intersect(vorigin, vray, gmin, gmax);

      float lmin, lmax;
      if (hit) 
        hit = lbox->intersect(vorigin, vray, lmin, lmax);

      float d = fabs(lmin) - fabs(gmin);
      if (hit && (lmax >= 0) && (d < fuzz) && (d > -fuzz))
      {
        rayList->set_x(kk, rayList->get_x(k));
        rayList->set_y(kk, rayList->get_y(k));
        rayList->set_ox(kk, vorigin.x);
        rayList->set_oy(kk, vorigin.y);
        rayList->set_oz(kk, vorigin.z);
        rayList->set_dx(kk, vray.x);
        rayList->set_dy(kk, vray.y);
        rayList->set_dz(kk, vray.z);
        rayList->set_r(kk, 0);
        rayList->set_g(kk, 0);
        rayList->set_b(kk, 0);
        rayList->set_o(kk, 0);
        rayList->set_t(kk, 0);
        rayList->set_tMax(kk, FLT_MAX);
        rayList->set_type(kk, RAY_PRIMARY);
        kk ++;
      }
    }

    if (kk)
    {
      if (kk < k)
        rayList->Truncate(kk);

#ifdef GXY_EVENT_TRACKING
      GetTheEventTracker()->Add(new InitialRaysEvent(rayList));
#endif

      renderingSet->Enqueue(rayList, true);
      renderer->add_originated_ray_count(rayList->GetRayCount());
    }
    else
      delete rayList;
  }
  else
  {
    int xknt = (ixmax - ixmin) + 1;
    int yknt = (iymax - iymin) + 1;

    int totalRays = xknt * yknt;
    if (totalRays <= 0)
      return;

    int rpp, n_packets;


    // approximate how many packets, then get the number
    // of rays in each

    if (totalRays > rays_per_packet)
    {
      n_packets = (totalRays / rays_per_packet) + 1;

      // Make sure there's no fragmentary final packet:
      // n_threads * rays_per_packet will be > totalRays
      // but totalRays - (n_threads * rays_per_packet) < n_packets

      rpp = ((totalRays + (n_packets-1)) / n_packets);
    }
    else
    {
      n_packets = 1;
      rpp = totalRays;
    }

    int iwidth  = (ixmax - ixmin) + 1;
    int iheight = (iymax - ixmin) + 1;

    ThreadPool *threadpool = GetTheApplication()->GetTheThreadPool();
    shared_ptr<spawn_rays_args> a = shared_ptr<spawn_rays_args>(new spawn_rays_args(
      fnum, pixel_scaling, iwidth, 
      ixmin, iymin,
      off_x, off_y, 
      vr, vu, veye, center, 
      lbox, gbox, 
      renderer, renderingSet, 
      rendering, this));

      for (int i = 0; i < (iwidth * iheight); i += rays_per_packet)
      {
#ifdef GXY_WRITE_IMAGES
        renderingSet->IncrementActiveCameraCount();     // Matching Decrement in thread
#endif
        int kthis = (i + rays_per_packet) > (iwidth * iheight) ? (iwidth * iheight) - i : rays_per_packet;
        rvec.emplace_back(threadpool->AddTask(new spawn_rays_task(i, kthis, a)));
      }
  }

  return;
}

void
Camera::print()
{
  std::stringstream s;
  s << "eye: " << eye[0] << " " << eye[1] << " " << eye[2] << endl;
  s << "dir: " << dir[0] << " " << dir[1] << " " << dir[2] << endl;
  s << "up: "  << up[0] << " " << up[1] << " " << up[2] << endl;
  s << "aov: " << aov;
  GetTheApplication()->Print(s.str());
}

int 
Camera::serialSize()
{
  return KeyedObject::serialSize() + (sizeof(int) + annotation.length() + 1) + sizeof(eye) + sizeof(dir) + sizeof(up) + sizeof(float) + sizeof(int) + sizeof(int);
}

unsigned char *
Camera::serialize(unsigned char *p)
{
  int l = annotation.length() + 1;
  *(int *)p = l;
  p += sizeof(l);

  memcpy(p, annotation.c_str(), l-1);
  p[l-1] = 0;
  p += l;

  memcpy(p, eye, sizeof(eye));
  p += sizeof(eye);

  memcpy(p, dir, sizeof(dir));
  p += sizeof(dir);

  memcpy(p, up, sizeof(up));
  p += sizeof(up);

  *(float *)p = aov;
  p += sizeof(float);

  *(int *)p = camwidth;
  p += sizeof(int);

  *(int *)p = camheight;
  p += sizeof(int);

  return p;
}

unsigned char *
Camera::deserialize(unsigned char *p)
{
  int l = *(int *)p;
  p += sizeof(l);
  annotation = (char *)p;
  p += l;

  set_viewpoint((float *)p);
  p += 3*sizeof(float);

  set_viewdirection((float *)p);
  p += 3*sizeof(float);

  set_viewup((float *)p);
  p += 3*sizeof(float);

  set_angle_of_view(*(float *)p);
  p += sizeof(float);

  set_width(*(int *)p);
  p += sizeof(int);

  set_height(*(int *)p);
  p += sizeof(int);

  return p;
}

} // namespace gxy
