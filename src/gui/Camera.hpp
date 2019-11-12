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
#include "dtypes.h"

#include <QJsonObject>

class CameraDialog;

class Camera
{
  friend class CameraDialog;

public:
  Camera()
  {
    point = {0.0, 0.0, -5.0};
    direction = {0.0, 0.0, 1.0};
    up = {0.0, 1.0, 0.0};
    aov = 30.0;
    size = {512, 512};
  }
  
  Camera(gxy::vec3f p, gxy::vec3f d, gxy::vec3f u, float a, gxy::vec2i s)
  {
    point = p;
    direction = d;
    up = u;
    aov = a;
    size = s;
  }
  
  ~Camera() {}

  gxy::vec3f getPoint()     { return point; }
  gxy::vec3f getDirection() { return direction; }
  gxy::vec3f getUp()        { return up; }
  float getAOV()            { return aov; }
  gxy::vec2i getSize()      { return size; }

  void setPoint(gxy::vec3f p) { point = p; }
  void setPoint(float x, float y, float z) { point.x = x; point.y = y; point.z = z; }

  void setDirection(gxy::vec3f d) { direction = d; }
  void setDirection(float x, float y, float z) { direction.x = x; direction.y = y; direction.z = z; }

  void setUp(gxy::vec3f u) { up = u; }
  void setUp(float x, float y, float z) { up.x = x; up.y = y; up.z = z; }

  void setAOV(float f)       { aov = f; }

  void setSize(int w, int h) { size.x = w; size.y = h; }

  virtual QJsonObject save() const
  {
    QJsonObject modelJson;

    QJsonObject pointJson;
    pointJson["x"] = point.x;
    pointJson["y"] = point.y;
    pointJson["z"] = point.z;
    modelJson["point"] = pointJson;

    QJsonObject directionJson;
    directionJson["x"] = direction.x;
    directionJson["y"] = direction.y;
    directionJson["z"] = direction.z;
    modelJson["direction"] = directionJson;

    QJsonObject upJson;
    upJson["x"] = up.x;
    upJson["y"] = up.y;
    upJson["z"] = up.z;
    modelJson["up"] = upJson;

    QJsonObject sizeJson;
    pointJson["w"] = size.x;
    pointJson["h"] = size.y;
    modelJson["size"] = sizeJson;

    modelJson["angle of view"] = aov;

    return modelJson;
  }

  virtual void restore(QJsonObject const& p) 
  {
    point.x = p["point"]["x"].toDouble();
    point.y = p["point"]["y"].toDouble();
    point.z = p["point"]["z"].toDouble();

    direction.x = p["direction"]["x"].toDouble();
    direction.y = p["direction"]["y"].toDouble();
    direction.z = p["direction"]["z"].toDouble();

    up.x = p["up"]["x"].toDouble();
    up.y = p["up"]["y"].toDouble();
    up.z = p["up"]["z"].toDouble();

    size.x = p["size"]["w"].toInt();
    size.y = p["size"]["h"].toInt();

    aov = p["angle of view"].toDouble();
  }

protected:
  gxy::vec3f point;
  gxy::vec3f direction;
  gxy::vec3f up;
  gxy::vec2i size;
  float aov;
};
