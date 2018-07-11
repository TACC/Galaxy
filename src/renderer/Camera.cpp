#define FUZZ 0.000001
#define LOGGING 0 

#include <string.h>
#include <string>
#include <sstream>
#include <iostream>
#include <math.h>
#include <float.h>
#include "tbb/tbb.h"
#include "Application.h"
#include "Renderer.h"
#include "MessageManager.h"
#include "Camera.h"
#include "Rays.h"
#include "RayFlags.h"
#include "Rendering.h"

using namespace rapidjson;
using namespace std;

namespace gxy
{
KEYED_OBJECT_TYPE(Camera)

static int RAYS_PER_PACKET = -1;

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
    rset = rayList->GetTheRendering()->GetTheRenderingSetKey();
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

struct args
{
  args(float pixel_scaling, float ox, float oy, vec3f& r, vec3f& u, vec3f& e, vec3f c, Box *lb, Box *gb) :
   scaling(1.0 / pixel_scaling), off_x(ox), off_y(oy), vr(r), vu(u), veye(e), center(c), lbox(lb), gbox(gb) {}
  ~args() {}

  float scaling;
  float off_x, off_y;
  vec3f vr, vu, veye, center;
  Box *lbox, *gbox;
};

#if 0
extern unsigned long tacc_rdtscp(int *chip, int *core);
#endif

class gil_ftor
{
public:
  gil_ftor(RayList *_r, shared_ptr<args> _a) : r(_r), a(_a) {}
  ~gil_ftor() {}

  virtual void operator()()
  {
#if 0
    int chip, core;
    tacc_rdtscp(&chip, &core);
    std::cerr << chip << "::" << core << "\n";
#endif

    int dst = 0;

    Box *lb = a->lbox;
    Box *gb = a->gbox;

    for (int i = 0, j = 0; i < r->GetRayCount(); i++)
    {
      vec3f xy_wcs, vray;

      float fx = (r->get_x(i) - a->off_x) * a->scaling;
      float fy = (r->get_y(i) - a->off_y) * a->scaling;

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
				r->set_x(dst, r->get_x(i));
				r->set_y(dst, r->get_y(i));
				r->set_ox(dst, a->veye.x);
        r->set_oy(dst, a->veye.y);
        r->set_oz(dst, a->veye.z);
        r->set_dx(dst, vray.x);
        r->set_dy(dst, vray.y);
        r->set_dz(dst, vray.z);
        r->set_r(dst, 0);
        r->set_g(dst, 0);
        r->set_b(dst, 0);
        r->set_o(dst, 0);
        r->set_t(dst, 0);
        r->set_tMax(dst, FLT_MAX);
        r->set_type(dst, RAY_PRIMARY);
        dst++;
      }
    }

    if (dst > 0)
    {
			if (dst < r->GetRayCount())
				r->Truncate(dst);

#if defined(EVENT_TRACKING)
      GetTheEventTracker()->Add(new InitialRaysEvent(r));
#endif
      r->GetTheRendering()->GetTheRenderingSet()->Enqueue(r, true);
    }
    else
      delete r;
  }

private:
  RayList *r;
  shared_ptr<args> a;
};

class wrapper
{
public:
  wrapper(shared_ptr<gil_ftor> _f) : f(_f)  {}
  ~wrapper() {}
  void operator()() { (*f)(); }
private:
  shared_ptr<gil_ftor> f;
};


void
Camera::generate_initial_rays(RenderingP rendering, Box* lbox, Box *gbox, vector<future<void>>& rvec)
{
  int width, height;
  rendering->GetTheSize(width, height);

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
  
  if (getenv("FULLWINDOW"))
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
  
	// std::cerr << "X";

  if (getenv("RAYDEBUG"))
  {
    int Ymin, Ymax;
    if (getenv("Y")) Ymin = Ymax = atoi(getenv("Y"));
    else
    {
      Ymin = getenv("YMIN") ? atoi(getenv("YMIN")) : (height / 2) - 2;
      Ymax = getenv("YMAX") ? atoi(getenv("YMAX")) : (height / 2) + 2;
      if (Ymin < 0) Ymin = 0;
      if (Ymax >= (height-1)) Ymax = height-1;
    }

    int Xmin, Xmax;
    if (getenv("X")) Xmin = Xmax = atoi(getenv("X"));
    else
    {
      Xmin = getenv("XMIN") ? atoi(getenv("XMIN")) : (width / 2) - 2;
      Xmax = getenv("XMAX") ? atoi(getenv("XMAX")) : (width / 2) + 2;
      if (Xmin < 0) Xmin = 0;
      if (Xmax >= (width-1)) Xmax = width-1;
    }

#if 1
    if ((Xmin > ixmax) || (Xmax < ixmin) || (Ymin > iymax) || (Ymax < iymin))
      return;

    if (Xmin < ixmin) Xmin = ixmin;
    if (Xmax > ixmax) Xmax = ixmax;
    if (Ymin < iymin) Ymin = iymin;
    if (Ymax > iymax) Ymax = iymax;
#endif

    int nrays = ((Ymax-Ymin) + 1) * ((Xmax-Xmin) + 1);
    if (nrays < 0)
      return;

    RayList *rayList = new RayList(rendering, nrays);

    vec2i *test_rays = new vec2i[nrays];

    int kk = 0;
    for (int y = 0; y < (Ymax-Ymin) + 1; y++)
      for (int x = 0; x < (Xmax-Xmin) + 1; x++)
      {
				rayList->set_x(kk, Xmin + x);
				rayList->set_y(kk, Ymin + y);
				kk++;
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

#if defined(EVENT_TRACKING)
			GetTheEventTracker()->Add(new InitialRaysEvent(rayList));
#endif

			rayList->GetTheRendering()->GetTheRenderingSet()->Enqueue(rayList, true);
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

    int rays_per_packet;
    int n_packets;

    if (RAYS_PER_PACKET == -1)
    {
      if (getenv("RAYS_PER_PACKET"))
        RAYS_PER_PACKET = atoi(getenv("RAYS_PER_PACKET"));
      else
        RAYS_PER_PACKET = 10000000;
    }

    // approximate how many packets, then get the number
    // of rays in each

    if (totalRays > RAYS_PER_PACKET)
    {
      n_packets = (totalRays / RAYS_PER_PACKET) + 1;

      // Make sure there's no fragmentary final packet:
      // n_threads * rays_per_packet will be > totalRays
      // but totalRays - (n_threads * rays_per_packet) < n_packets

      rays_per_packet = ((totalRays + (n_packets-1)) / n_packets);
    }
    else
    {
      n_packets = 1;
      rays_per_packet = totalRays;
    }

    ThreadPool *threadpool = GetTheApplication()->GetTheThreadPool();
    shared_ptr<args> a = shared_ptr<args>(new args(pixel_scaling, off_x, off_y, vr, vu, veye, center, lbox, gbox));

    RayList *rlist = NULL;    // Null first time
    int knt = 0;

    for (int y = iymin, knt_in_pkt = 0, tot_knt = 0; y <= iymax; y++)
      for (int x = ixmin; x <= ixmax; x++, knt_in_pkt++, tot_knt++)
      {
        if (!rlist || knt_in_pkt == rlist->GetRayCount())
        {
          if (rlist)
          {
            shared_ptr<gil_ftor> f = shared_ptr<gil_ftor>(new gil_ftor(rlist, a));
            rvec.emplace_back(threadpool->postWork<void>(wrapper(f)));
          }

          if (tot_knt + rays_per_packet > totalRays)
            rlist = new RayList(rendering, totalRays - tot_knt);
          else
            rlist = new RayList(rendering, rays_per_packet);

          knt_in_pkt = 0;
        }
          
				rlist->set_x(knt, x);
				rlist->set_y(knt, y);
        knt ++;
      }

      if (rlist)
      {
        shared_ptr<gil_ftor> f = shared_ptr<gil_ftor>(new gil_ftor(rlist, a));
        rvec.emplace_back(threadpool->postWork<void>(wrapper(f)));
      }
  }
  return;
}

void
Camera::print()
{
  std::stringstream s;
  s << "eye: " << eye[0] << " " << eye[1] << " " << eye[2] << "\n";
  s << "dir: " << dir[0] << " " << dir[1] << " " << dir[2] << "\n";
  s << "up: "  << up[0] << " " << up[1] << " " << up[2] << "\n";
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
}