// ========================================================================== //
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

#define FUZZ 0.000001

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

using namespace rapidjson;
using namespace std;

namespace gxy
{

KEYED_OBJECT_TYPE(Camera)

vector<int> permutation;
int rays_per_packet;
bool full_window;
bool raydebug;
static int Xmax, Xmin, Ymax, Ymin;
bool permute;

vector<int>
generate_permutation(int num)
{
  std::srand (unsigned(std::time(0)));
  std::vector<int> p;
  for (int i=1; i<num; ++i) p.push_back(i);
  std::random_shuffle (p.begin(), p.end() );
  return p;
}

void
Camera::Register()
{
  RegisterClass();
}

vector<CameraP>
Camera::LoadCamerasFromJSON(Value& v)
{
  vector<CameraP> cameras;

  if (v.HasMember("Camera") || v.HasMember("Cameras"))
  {
    Value& c = v.HasMember("Camera") ? v["Camera"] : v["Cameras"];
    if (c.IsArray())
    {
      for (int i = 0; i < c.Size(); i++)
      {
        CameraP cam = Camera::NewP();
        cam->LoadFromJSON(c[i]);

        cameras.push_back(cam);
      }
    }
    else
    {
      CameraP cam = Camera::NewP();
      cam->LoadFromJSON(c);
      cameras.push_back(cam);
    }
  }
  else
    std::cerr << "No cameras found\n";

  return cameras;
}

void
Camera::SaveToJSON(Value&v, Document& doc)
{
  Value c(kObjectType);

  Value e(kArrayType);
  e.PushBack(Value().SetDouble(eye[0]), doc.GetAllocator());
  e.PushBack(Value().SetDouble(eye[1]), doc.GetAllocator());
  e.PushBack(Value().SetDouble(eye[2]), doc.GetAllocator());
  c.AddMember("viewpoint", e, doc.GetAllocator());

  Value d(kArrayType);
  d.PushBack(Value().SetDouble(dir[0]), doc.GetAllocator());
  d.PushBack(Value().SetDouble(dir[1]), doc.GetAllocator());
  d.PushBack(Value().SetDouble(dir[2]), doc.GetAllocator());
  c.AddMember("viewdirection", d, doc.GetAllocator());

  Value u(kArrayType);
  u.PushBack(Value().SetDouble(up[0]), doc.GetAllocator());
  u.PushBack(Value().SetDouble(up[1]), doc.GetAllocator());
  u.PushBack(Value().SetDouble(up[2]), doc.GetAllocator());
  c.AddMember("viewup", u, doc.GetAllocator());

  c.AddMember("aov", Value().SetDouble(aov), doc.GetAllocator());

  v.AddMember("Camera", c, doc.GetAllocator());
}

void 
Camera::LoadFromJSON(Value& v)
{
	if (v.IsString())
	{
    ifstream ifs(v.GetString());
    stringstream ss;
    ss << ifs.rdbuf();

    Document tf;
    tf.Parse(ss.str().c_str());

		float center[3];

		Value& properties = tf["PVCameraConfiguration"]["Proxy"]["Property"];
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
			SetAnnotation(string(v["annotation"].GetString()));

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
				exit(1);
		}

		up[0] = v["viewup"][0].GetDouble();
		up[1] = v["viewup"][1].GetDouble();
		up[2] = v["viewup"][2].GetDouble();

		aov = v["aov"].GetDouble();
	}
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
intersect_line_plane(vec3f& p0, vec3f& p1, vec3f& plane_xyz, float plane_w, vec3f& x)
{
  float W = (p0 * plane_xyz) + plane_w;
  vec3f V = (p0 - plane_xyz * W);

  vec3f w = p0 - V;
  vec3f u = p1 - p0;
  float N = -(plane_xyz * w);
  float D = plane_xyz * u;
  if (fabs(D) < 1e-6) return false;
  else
  {
    x = p0 + (u * (N / D));
    return true;
  }
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

struct args
{
  args(int fnum, float pixel_scaling, int iw, int ixmin, int iymin, float ox, float oy, 
			 vec3f& vr, vec3f& vu, vec3f& veye, vec3f center, 
			 Box *lb, Box *gb, RenderingSetP rs, RenderingP r) :
			 fnum(fnum), iwidth(iw), ixmin(ixmin), iymin(iymin),
			 scaling(1.0 / pixel_scaling), off_x(ox), off_y(oy),
			 vr(vr), vu(vu), veye(veye), center(center), lbox(lb), gbox(gb),
			 rs(rs), r(r) {}
  ~args() {}

	int fnum;
  float scaling;
	int iwidth, ixmin, iymin;
  float off_x, off_y;
  vec3f vr, vu, veye, center;
  Box *lbox, *gbox;
	RenderingSetP rs;
	RenderingP r;
};

// Given a slew of camera parameters and a range of pixels that MIGHT
// be first-hit, figure out which actually are, create a raylist for them,
// and queue them for processing

class spawn_rays_task : public ThreadPoolTask
{
public:
  spawn_rays_task(int start, int count, shared_ptr<args> _a) : start(start), count(count), a(_a) {}
  ~spawn_rays_task() {}

  virtual int work()
  {
#if defined(EVENT_TRACKING)
		GetTheEventTracker()->Add(new CameraTaskStartEvent());
#endif

		RayList *rlist = NULL;

    int dst = 0;
    for (int i = 0; i < count; i++)
    {
			int pindex = i + start;
			int x, y;
      vec3f xy_wcs, vray;

			if (permute)
			{
		    int p = permutation[pindex];
				x = a->ixmin + (p % a->iwidth);
				y = a->iymin + (p / a->iwidth);
			}
			else
			{
				x = a->ixmin + (pindex % a->iwidth);
				y = a->iymin + (pindex / a->iwidth);
			}

      float fx = (x - a->off_x) * a->scaling;
      float fy = (y - a->off_y) * a->scaling;

      xy_wcs.x = a->center.x + fx * a->vr.x + fy * a->vu.x;
      xy_wcs.y = a->center.y + fx * a->vr.y + fy * a->vu.y;
      xy_wcs.z = a->center.z + fx * a->vr.z + fy * a->vu.z;

      vray = xy_wcs - a->veye;
      normalize(vray);

      float gmin, gmax;
      bool hit = a->gbox->intersect(a->veye, vray, gmin, gmax);

      float lmin, lmax;
      if (hit) hit = a->lbox->intersect(a->veye, vray, lmin, lmax);

      float d = fabs(lmin) - fabs(gmin);
      if (hit && (lmax >= 0) && (d < FUZZ) && (d > -FUZZ))
      {
				if (!rlist) rlist = new RayList(a->rs, a->r, count, a->fnum);

				rlist->set_x(dst, x);
				rlist->set_y(dst, y);
				rlist->set_ox(dst, a->veye.x);
        rlist->set_oy(dst, a->veye.y);
        rlist->set_oz(dst, a->veye.z);
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
			if (a->fnum == a->rs->GetCurrentFrame())
		  {
				if (dst < rlist->GetRayCount())
					rlist->Truncate(dst);

#ifdef GXY_EVENT_TRACKING
				GetTheEventTracker()->Add(new InitialRaysEvent(rlist));
#endif

				Renderer::GetTheRenderer()->add_originated_ray_count(rlist->GetRayCount());
				a->rs->Enqueue(rlist, true);
			}
			else
				delete rlist;
		}

#ifdef GXY_SYNCHRONOUS
		a->rs->DecrementActiveCameraCount(dst);				
#endif

#ifdef GXY_EVENT_TRACKING
		GetTheEventTracker()->Add(new CameraTaskEndEvent());
#endif

		return 0;
  }

private:
	int start, count;
  shared_ptr<args> a;
};

void check_env(int width, int height)
{
	static bool first = true;

	if (first)
	{
		first = false;

		full_window =  getenv("GXY_FULLWINDOW") != NULL;
		raydebug    =  getenv("GXY_RAYDEBUG") != NULL;

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

		rays_per_packet = Renderer::GetTheRenderer()->GetMaxRayListSize();
		permute = getenv("GXY_PERMUTE_PIXELS");

		if (permute)
		{
			std::cerr << "permute pixels\n";
			permutation = generate_permutation(width*height);
		}
		else
			std::cerr << "NO permute pixels\n";

    if (getenv("GXY_CAMERA_RAYS_PER_PIXEL"))
       rays_per_packet = atoi(getenv("CAMERA_RAYS_PER_PIXEL"));
	}
}
		
void
Camera::generate_initial_rays(RenderingSetP renderingSet, RenderingP rendering, Box* lbox, Box *gbox, vector<future<int>>& rvec, int fnum)
{
  int width, height;
  rendering->GetTheSize(width, height);

	check_env(width, height);

  vec3f veye(eye);
  vec3f vu(up);
  vec3f vdir(dir);
  normalize(vdir);

  // Center is the center of the image plane in WCS

  float d = 1.0 / tan(2*3.1415926*(aov/2.0)/360.0);
  vec3f center = vec3f(eye) + (vdir * d);

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
      intersect_line_plane(veye, corner, vdir, w, proj);
    
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

    RayList *rayList = new RayList(renderingSet, rendering, nrays, fnum);

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

      vec3f xy_wcs, vray;
      float fx = (x - off_x) / pixel_scaling;
      float fy = (y - off_y) / pixel_scaling;
      xy_wcs.x = center.x + fx * vr.x + fy * vu.x;
      xy_wcs.y = center.y + fx * vr.y + fy * vu.y;
      xy_wcs.z = center.z + fx * vr.z + fy * vu.z;
      vray = xy_wcs - veye;
      normalize(vray);

      float gmin, gmax;
      bool hit = gbox->intersect(veye, vray, gmin, gmax);

      float lmin, lmax;
      if (hit) hit = lbox->intersect(veye, vray, lmin, lmax);

      float d = fabs(lmin) - fabs(gmin);
      if (hit && (lmax >= 0) && (d < FUZZ) && (d > -FUZZ))
      {
        rayList->set_x(kk, rayList->get_x(k));
        rayList->set_y(kk, rayList->get_y(k));
        rayList->set_ox(kk, veye.x);
        rayList->set_oy(kk, veye.y);
        rayList->set_oz(kk, veye.z);
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

			rayList->GetTheRenderingSet()->Enqueue(rayList, true);
			Renderer::GetTheRenderer()->add_originated_ray_count(rayList->GetRayCount());
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
    shared_ptr<args> a = shared_ptr<args>(new args(fnum, pixel_scaling, iwidth, 
																									 ixmin, iymin,
																									 off_x, off_y, 
																									 vr, vu, veye, center, 
																									 lbox, gbox, renderingSet, rendering));

                for (int i = 0; i < (iwidth * iheight); i += rays_per_packet)
                {
#ifdef GXY_SYNCHRONOUS
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
  return KeyedObject::serialSize() + (sizeof(int) + annotation.length() + 1) + sizeof(eye) + sizeof(dir) + sizeof(up) + sizeof(float);
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

  return p;
}

} // namespace gxy
