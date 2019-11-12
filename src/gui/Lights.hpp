// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
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

#include <iostream> 
#include <vector> 
#include "dtypes.h"

#include <QJsonObject>
#include <QJsonArray>

class LightDialog;

class Light
{
  friend class LightDialog;

public:
  Light()
  {
    point = {1.0, 1.0, 1.0};
    type = 0;
  }
  
  Light(gxy::vec3f p, int t)
  {
    point = p;
    type = t;
  }
  
  ~Light() {}

  gxy::vec3f get_point()   { return point; }
  int get_type()           { return type; }

  void set_point(gxy::vec3f p) { point = p; }
  void set_point(float x, float y, float z) { point.x = x; point.y = y; point.z = z; }

  virtual QJsonObject save() const
  {
    QJsonObject modelJson;

    modelJson["x"] = point.x;
    modelJson["y"] = point.x;
    modelJson["z"] = point.x;
    modelJson["type"] = type;

    return modelJson;
  }

  virtual void restore(QJsonObject const& p)
  {
    point.x = p["x"].toDouble();
    point.y = p["y"].toDouble();
    point.z = p["z"].toDouble();
    type = p["type"].toInt();
  }

  void set_type(int t)       { type = t; }

  gxy::vec3f point;
  int type;
};

class LightingEnvironment 
{
public:
  LightingEnvironment()
  {
    lights.push_back(Light(gxy::vec3f(1.0, 1.0, 0.0), 0));
    shadow_flag = false;
    ao_flag = false;
    ao_count = 16;
    ao_radius = 1.0;
    Ka = 0.4;
    Kd = 0.6;
  }

  ~LightingEnvironment() {}

  virtual QJsonObject save() const
  {
    QJsonObject modelJson;

    modelJson["shadow flag"] = shadow_flag ? 1 : 0;
    modelJson["ao flag"] = ao_flag ? 1 : 0;
    modelJson["ao count"] = ao_count;
    modelJson["ao radius"] = ao_radius;
    modelJson["Ka"] = Ka;
    modelJson["Kd"] = Kd;
    
    QJsonArray lightsJson;
    for (auto l : lights)
      lightsJson.push_back(l.save());

    modelJson["lights"] = lightsJson;

    return modelJson;
  }

  virtual void restore(QJsonObject const& p)
  {
    shadow_flag = p["shadow flag"].toInt() == 1;
    ao_flag = p["ao flag"].toInt() == 1;
    ao_count = p["ao count"].toInt();
    ao_radius = p["ao radius"].toDouble();
    Ka = p["Ka"].toDouble();
    Kd = p["Kd"].toDouble();

    lights.clear();

    for (auto lightJson : p["lights"].toArray())
    {
      gxy::vec3f pt;
      int t;

      pt.x = lightJson.toObject()["x"].toDouble();
      pt.y = lightJson.toObject()["y"].toDouble();
      pt.z = lightJson.toObject()["z"].toDouble();
      t = lightJson.toObject()["type"].toInt();
      
      Light l(pt, t);
      lights.push_back(l);
    }
  }
  
  std::vector<Light> lights;
  bool shadow_flag, ao_flag;
  int  ao_count;
  float ao_radius;
  float Ka, Kd;
};
