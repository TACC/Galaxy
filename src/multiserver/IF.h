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

#include <string>
#include <vector>

#include "MultiServerSocket.h"
#include "dtypes.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

class IF
{
public:
  void LoadString(const char *s) { rapidjson::Document doc; doc.Parse(s); Load(doc); }
  void Send(gxy::MultiServerSocket* skt)
  {
    std::string s(Stringify());
    if (! skt->CSendRecv(s))
    {
      std::cerr << "IF sendrecv failed\n";
      exit(1);
    }
    // std::cerr << "IF reply: " << s << "\n";
  }
  
  virtual void Load(rapidjson::Value&) = 0;
  virtual void Print() = 0;
  virtual char *Stringify() = 0;
};

class DatasetsIF : public IF
{
  struct Dataset
  {
    Dataset(std::string n, std::string t, std::string f) : name(n), type(t), filename(f) {}
    std::string name;
    std::string type;
    std::string filename;
  };

public:
  void Print();
  void Load(rapidjson::Value&);
  void Send(gxy::MultiServerSocket& skt);
  char *Stringify();

  void Add(std::string n, std::string t, std::string f);

private:
  std::vector<Dataset> datasets;
};

class CameraIF : public IF
{
public:
  void Print();
  void Load(rapidjson::Value&);
  char *Stringify();

  //! set the spatial location for this camera
  void set_viewpoint(float x, float y, float z) { eye[0] = x; eye[1] = y; eye[2] = z; }
  //! set the spatial location for this camera
  void set_viewpoint(float *xyz)  { eye[0] = xyz[0]; eye[1] = xyz[1]; eye[2] = xyz[2]; }
  //! set the spatial location for this camera
  void set_viewpoint(gxy::vec3f xyz)  { eye[0] = xyz.x; eye[1] = xyz.y; eye[2] = xyz.z; }


  //! get the spatial location for this camera
  void get_viewpoint(float& x, float& y, float& z) { x = eye[0]; y = eye[1]; z = eye[2]; }
  //! get the spatial location for this camera
  void get_viewpoint(float *xyz)  { xyz[0] = eye[0]; xyz[1] = eye[1]; xyz[2] = eye[2]; }
  //! get the spatial location for this camera
  void get_viewpoint(gxy::vec3f& xyz)  { xyz.x = eye[0]; xyz.y = eye[1]; xyz.z = eye[2]; }


  //! set the view direction for the camera
  void set_viewdirection(float x, float y, float z);
  //! set the view direction for the camera
  void set_viewdirection(float *xyz)  { dir[0] = xyz[0]; dir[1] = xyz[1]; dir[2] = xyz[2]; }
  //! set the view direction for the camera
  void set_viewdirection(gxy::vec3f xyz)  { dir[0] = xyz.x; dir[1] = xyz.y; dir[2] = xyz.z; }


  //! get the view direction for the camera
  void get_viewdirection(float &x, float &y, float &z) { x = dir[0]; y = dir[1]; z = dir[2]; }
  //! get the view direction for the camera
  void get_viewdirection(float *xyz)  { xyz[0] = dir[0]; xyz[1] = dir[1]; xyz[2] = dir[2]; }
  //! get the view direction for the camera
  void get_viewdirection(gxy::vec3f& xyz)  { xyz.x = dir[0]; xyz.y = dir[1]; xyz.z = dir[2]; }

  //! set the up orientation for the camera
  void set_viewup(float x, float y, float z);
  //! set the up orientation for the camera
  void set_viewup(float *xyz)  { up[0] = xyz[0]; up[1] = xyz[1]; up[2] = xyz[2]; }
  //! set the up orientation for the camera
  void set_viewup(gxy::vec3f xyz)  { up[0] = xyz.x; up[1] = xyz.y; up[2] = xyz.z; }

  //! get the up orientation for the camera
  void get_viewup(float &x, float &y, float &z) { x = up[0]; y = up[1]; z = up[2]; }
  //! get the up orientation for the camera
  void get_viewup(float *xyz)  { xyz[0] = up[0]; xyz[1] = up[1]; xyz[2] = up[2]; }
  //! get the up orientation for the camera
  void get_viewup(gxy::vec3f& xyz)  { xyz.x = up[0]; xyz.y = up[1]; xyz.z = up[2]; }

  //! set the view angle for the camera
  void set_angle_of_view(float a) { aov = a; }
  //! get the view angle for the camera
  void get_angle_of_view(float &a) { a = aov; }

private:
  int   size[2];
  float eye[3];
  float dir[3];
  float up[3];
  float aov;
};
