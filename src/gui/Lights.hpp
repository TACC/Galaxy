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

  virtual QJsonArray save() const
  {
    QJsonArray modelJson;

    modelJson.push_back(point.x);
    modelJson.push_back(point.y);
    modelJson.push_back(point.z);
    modelJson.push_back(type);

    return modelJson;
  }

  virtual void restore(QJsonArray const& p)
  {
    point.x = p[0].toDouble();
    point.y = p[1].toDouble();
    point.z = p[2].toDouble();
    type = p[3].toInt();
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
    lights.push_back(Light(gxy::vec3f(1.0, -1.0, 1.0), 0));
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

    modelJson["shadows"] = shadow_flag;
    modelJson["ao count"] = ao_flag ? ao_count : 0;
    modelJson["ao radius"] = ao_radius;
    modelJson["Ka"] = Ka;
    modelJson["Kd"] = Kd;
    
    QJsonArray lightsJson;
    for (auto l : lights)
      lightsJson.push_back(l.save());

    modelJson["Sources"] = lightsJson;

    return modelJson;
  }

  virtual void restore(QJsonObject const& p)
  {
    shadow_flag = p["ShadowFlag"].toBool();
    ao_count = p["ao count"].toInt();
    ao_radius = p["ao radius"].toDouble();
    ao_flag = ao_count > 0;
    Ka = p["Ka"].toDouble();
    Kd = p["Kd"].toDouble();

    lights.clear();

    for (auto lightJson : p["Sources"].toArray())
    {
      gxy::vec3f pt;
      int t;

      pt.x = lightJson.toArray()[0].toDouble();
      pt.y = lightJson.toArray()[1].toDouble();
      pt.z = lightJson.toArray()[2].toDouble();
      t = lightJson.toArray()[3].toInt();
      
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
