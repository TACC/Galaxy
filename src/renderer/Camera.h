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

#pragma once

/*! \file Camera.h 
 * \brief a viewpoint from which images are rendered
 * \ingroup render
 */

#include <future>
#include <memory>
#include <string>
#include <vector>

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

#include "Box.h"
#include "dtypes.h"
#include "GalaxyObject.h"
#include "Threading.h"
#include "Work.h"

namespace gxy
{

OBJECT_POINTER_TYPES(Renderer)
OBJECT_POINTER_TYPES(Rendering)
OBJECT_POINTER_TYPES(RenderingSet)
OBJECT_POINTER_TYPES(Camera)

class Rays;
class Rendering;
class RayList;

//! a viewpoint from which images are rendered
/*! \ingroup render */
class Camera : public KeyedObject
{
	KEYED_OBJECT(Camera)

  struct spawn_rays_args
  { 
    spawn_rays_args(int fnum, float pixel_scaling, int iw, int ixmin, int iymin, float ox, float oy,
       vec3f& vr, vec3f& vu, vec3f& veye, vec3f center,
       Box *lb, Box *gb, RendererDPtr rndr, RenderingSetDPtr rs, RenderingDPtr r, Camera *c) :
       fnum(fnum), iwidth(iw), ixmin(ixmin), iymin(iymin),
       scaling(1.0 / pixel_scaling), off_x(ox), off_y(oy),
       vr(vr), vu(vu), veye(veye), center(center), lbox(lb), gbox(gb),
       renderer(rndr), rs(rs), r(r), camera(c) {}
    ~spawn_rays_args() {}

    int fnum;
    float scaling;
    int iwidth, ixmin, iymin;
    float off_x, off_y;
    vec3f vr, vu, veye, center;
    Box *lbox, *gbox;
    RendererDPtr renderer;
    RenderingSetDPtr rs;
    RenderingDPtr r;
    Camera *camera;
  };

  class spawn_rays_task : public ThreadPoolTask
  {
  public:
    spawn_rays_task(int start, int count, std::shared_ptr<spawn_rays_args> _a) :
              ThreadPoolTask(1), start(start), count(count), a(_a) {}

    ~spawn_rays_task() {}

    virtual int work();

  private:
    int start, count;
    std::shared_ptr<spawn_rays_args> a;
  };

public:

	//! construct a set of Cameras from a Galaxy JSON specification
	static bool LoadCamerasFromJSON(rapidjson::Value&, std::vector<CameraDPtr>&);
	
	//! construct a Camera from a Galaxy JSON specification
	virtual bool LoadFromJSON(rapidjson::Value&);

	//! construct a Camera from a Paraview PVCC
	bool LoadFromPVCC(const char *filename);

	//! set the spatial location for this camera
	void set_viewpoint(float x, float y, float z) { eye[0] = x; eye[1] = y; eye[2] = z; }
	//! set the spatial location for this camera
	void set_viewpoint(float *xyz)  { eye[0] = xyz[0]; eye[1] = xyz[1]; eye[2] = xyz[2]; }
	//! set the spatial location for this camera
	void set_viewpoint(vec3f xyz)  { eye[0] = xyz.x; eye[1] = xyz.y; eye[2] = xyz.z; }

	//! get the spatial location for this camera
	void get_viewpoint(float& x, float& y, float& z) { x = eye[0]; y = eye[1]; z = eye[2]; }
	//! get the spatial location for this camera
	void get_viewpoint(float *xyz)  { xyz[0] = eye[0]; xyz[1] = eye[1]; xyz[2] = eye[2]; }
	//! get the spatial location for this camera
	void get_viewpoint(vec3f& xyz)  { xyz.x = eye[0]; xyz.y = eye[1]; xyz.z = eye[2]; }

	//! set the view direction for the camera
	void set_viewdirection(float x, float y, float z);
	//! set the view direction for the camera
	void set_viewdirection(float *xyz)  { dir[0] = xyz[0]; dir[1] = xyz[1]; dir[2] = xyz[2]; }
	//! set the view direction for the camera
	void set_viewdirection(vec3f xyz)  { dir[0] = xyz.x; dir[1] = xyz.y; dir[2] = xyz.z; }

    //! get the view direction for the camera
	void get_viewdirection(float &x, float &y, float &z) { x = dir[0]; y = dir[1]; z = dir[2]; }
	//! get the view direction for the camera
	void get_viewdirection(float *xyz)  { xyz[0] = dir[0]; xyz[1] = dir[1]; xyz[2] = dir[2]; }
	//! get the view direction for the camera
	void get_viewdirection(vec3f& xyz)  { xyz.x = dir[0]; xyz.y = dir[1]; xyz.z = dir[2]; }

	//! set the up orientation for the camera
	void set_viewup(float x, float y, float z);
	//! set the up orientation for the camera
	void set_viewup(float *xyz)  { up[0] = xyz[0]; up[1] = xyz[1]; up[2] = xyz[2]; }
	//! set the up orientation for the camera
	void set_viewup(vec3f xyz)  { up[0] = xyz.x; up[1] = xyz.y; up[2] = xyz.z; }

    //! get the up orientation for the camera
	void get_viewup(float &x, float &y, float &z) { x = up[0]; y = up[1]; z = up[2]; }
	//! get the up orientation for the camera
	void get_viewup(float *xyz)  { xyz[0] = up[0]; xyz[1] = up[1]; xyz[2] = up[2]; }
	//! get the up orientation for the camera
	void get_viewup(vec3f& xyz)  { xyz.x = up[0]; xyz.y = up[1]; xyz.z = up[2]; }

	//! set the view angle for the camera
	void set_angle_of_view(float a) { aov = a; }
	//! get the view angle for the camera
	void get_angle_of_view(float &a) { a = aov; }
	//! get the view angle for the camera
	float get_angle_of_view() { return aov; }

    //! set width
    void set_width(int w) {camwidth = w;}
    //! set height
    void set_height(int h) {camheight = h;}
    //! get width
    int get_width() { return camwidth;}
    //! get height of camera
    int get_height() { return camheight;}

	//! print the details of this camera
	void print();

	//! create the primary rays for this camera 
	/*! the camera creates rays according to the resolution of the rendering. 
	 * The extent of the local bounding box (lbox) is projected onto the 
	 * Camera framebuffer and rays that will not intersect it are not generated.
	 * 
	 * @param renderer a pointer to the Renderer associated with this Camera
	 * @param renderingSet a pointer to the RenderingSet that contains Renderings for this Camera
	 * @param rendering a pointer to the Rendering for this Camera
	 * @param lbox a pointer to the Box bounding the local data extent (i.e. data at this node)
	 * @param gbox a pointer to the Box bounding the global data extent (i.e. data across all nodes)
	 * @param rvec a vector of return values for worker threads processing RayLists of these primary rays
	 * @param fnum the frame number for these rays
	 */
	void generate_initial_rays(RendererDPtr renderer, RenderingSetDPtr renderingSet, RenderingDPtr rendering, Box* lbox, Box *gbox, std::vector<std::future<int>>& rvec, int fnum);

	//! the size in bytes for the serialization of this Camera object
  virtual int serialSize();
  virtual unsigned char *serialize(unsigned char *ptr); //!< serialize this Camera to the given byte stream
  virtual unsigned char *deserialize(unsigned char *ptr); //!< deserialize a Camera from the given byte stream into this Camera object

  void  SetAnnotation(std::string a) { annotation = a; } //!< set the annotation string for this Camera
  const char *GetAnnotation() { return annotation.c_str(); } //!< return the annotation string for this Camera

protected:

  bool SpawnRays(std::shared_ptr<spawn_rays_args> a, int start, int count);

  std::string annotation;
	
  float eye[3];
  float dir[3];
  float up[3];
  float aov;
  int   camwidth=512;
  int   camheight=512;

  std::vector<int> permutation;
  bool permute;
  int rays_per_packet;
};

} // namespace gxy
