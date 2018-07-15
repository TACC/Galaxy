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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

#include "Box.h"
#include "dtypes.h"
#include "KeyedObject.h"
#include "Work.h"

namespace gxy
{

KEYED_OBJECT_POINTER(Rendering)
KEYED_OBJECT_POINTER(RenderingSet)
KEYED_OBJECT_POINTER(Camera)

class Rays;
class Rendering;
class RayList;

class Camera : public KeyedObject
{
	KEYED_OBJECT(Camera)

public:

	static vector<CameraP> LoadCamerasFromJSON(rapidjson::Value&);
	
	virtual void LoadFromJSON(rapidjson::Value&);
	virtual void SaveToJSON(rapidjson::Value&, rapidjson::Document&);

	// virtual bool local_commit(MPI_Comm);

	void set_viewpoint(float x, float y, float z) { eye[0] = x; eye[1] = y; eye[2] = z; }
	void set_viewpoint(float *xyz)  { eye[0] = xyz[0]; eye[1] = xyz[1]; eye[2] = xyz[2]; }
	void set_viewpoint(vec3f xyz)  { eye[0] = xyz.x; eye[1] = xyz.y; eye[2] = xyz.z; }

	void get_viewpoint(float& x, float& y, float& z) { x = eye[0]; y = eye[1]; z = eye[2]; }
	void get_viewpoint(float *xyz)  { xyz[0] = eye[0]; xyz[1] = eye[1]; xyz[2] = eye[2]; }
	void get_viewpoint(vec3f& xyz)  { xyz.x = eye[0]; xyz.y = eye[1]; xyz.z = eye[2]; }

	void set_viewdirection(float x, float y, float z);
	void set_viewdirection(float *xyz)  { dir[0] = xyz[0]; dir[1] = xyz[1]; dir[2] = xyz[2]; }
	void set_viewdirection(vec3f xyz)  { dir[0] = xyz.x; dir[1] = xyz.y; dir[2] = xyz.z; }

	void get_viewdirection(float &x, float &y, float &z) { x = dir[0]; y = dir[1]; z = dir[2]; }
	void get_viewdirection(float *xyz)  { xyz[0] = dir[0]; xyz[1] = dir[1]; xyz[2] = dir[2]; }
	void get_viewdirection(vec3f& xyz)  { xyz.x = dir[0]; xyz.y = dir[1]; xyz.z = dir[2]; }

	void set_viewup(float x, float y, float z);
	void set_viewup(float *xyz)  { up[0] = xyz[0]; up[1] = xyz[1]; up[2] = xyz[2]; }
	void set_viewup(vec3f xyz)  { up[0] = xyz.x; up[1] = xyz.y; up[2] = xyz.z; }

	void get_viewup(float &x, float &y, float &z) { x = up[0]; y = up[1]; z = up[2]; }
	void get_viewup(float *xyz)  { xyz[0] = up[0]; xyz[1] = up[1]; xyz[2] = up[2]; }
	void get_viewup(vec3f& xyz)  { xyz.x = up[0]; xyz.y = up[1]; xyz.z = up[2]; }

	void set_angle_of_view(float a) { aov = a; }
	void get_angle_of_view(float &a) { a = aov; }

	void print();

	void generate_initial_rays(RenderingSetP, RenderingP, Box*, Box*, vector<future<void>>& rvec);

  virtual int serialSize();
  virtual unsigned char *serialize(unsigned char *ptr);
  virtual unsigned char *deserialize(unsigned char *ptr);

  void  SetAnnotation(string a) { annotation = a; }
  const char *GetAnnotation() { return annotation.c_str(); }

protected:

  string annotation;
	
	float eye[3];
	float dir[3];
	float up[3];
	float aov;
};

} // namespace gxy
