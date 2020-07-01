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

#include <string>
#include <vector>

#include "SocketHandler.h"
#include "dtypes.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

/*! \file JsonInterface.h
 *  \brief Contains the definitions of the structures a viewer uses to cause 
 *         renderer to to update
 */

//! JsonInterface is the super class that knows how to transform to and from json
class JsonInterface
{
public:
  /*! Parse a JSON input string and call a virtual method to update 
   *  subcass class members
   */
  void LoadString(const char *s) { rapidjson::Document doc; doc.Parse(s); Load(doc); }

  /*! Call a virtual member to create a JSON string from the subclass members
   *  and ship them across the socket handler
   */
  void Send(gxy::SocketHandler* handler)
  {
    std::string s(Stringify());
    if (! handler->CSendRecv(s) || s.substr(0, 2) != "ok")
    {
      std::cerr << "JsonInterface sendrecv failed: " << s << "\n";
      exit(1);
    }
  }
  
  //! Virtual method to load subclass members from JSON object
  virtual void Load(rapidjson::Value&) = 0;
  
  //! Virtual method to print subclass members
  virtual void Print() = 0;
  
  //! Virtual method to create JSON string from subclass members
  virtual char *Stringify() = 0;
};

class DatasetsInterface : public JsonInterface
{
  //! Datasets interface : represents a set of data objects that the client would like the server to manage

  struct Dataset
  {
    //! a structure to specify a dataset: its name in the Galaxy context, its type, and its file name
    Dataset(std::string n, std::string t, std::string f) : name(n), type(t), filename(f) {}
    std::string name;
    std::string type;
    std::string filename;
  };

public:
  //! overloaded virtual function to print a Datasets IF object
  void Print();

  //! overloaded virtual function to load Datasets subclass members from JSON object
  void Load(rapidjson::Value&);

  //! overloaded virtual function to create a JSON string from a Datasets IF object
  char *Stringify();

  //! add a dataset to the list
  //! \param n - name of data object in the Galaxy context
  //! \param t - type of data - eg. particles, volume, mesh...
  //! \param f - filename pointing to data
  void Add(std::string n, std::string t, std::string f);

private:
  std::vector<Dataset> datasets;
};

class CameraInterface : public JsonInterface
{
  //! Camera interface : represents a camera of data objects that the client would like to send to the server

public:
  //! overloaded virtual function to print a Camera IF object
  void Print();

  //! overloaded virtual function to load Camera subclass members from JSON object
  void Load(rapidjson::Value&);

  //! overloaded virtual function to create a JSON string from a Camera IF object
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
