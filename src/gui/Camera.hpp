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
#include <QJsonArray>

class CameraDialog;

class Camera
{
  friend class CameraDialog;

public:
  Camera()
  {
    point = {0.0, 0.0, -5.0};
    direction = {0.0, 0.0, 5.0};
    up = {0.0, 1.0, 0.0};
    aov = 30.0;
    size = {512, 512};
  }

  void print()
  {
    std::cerr << "point: " << point.x << " " << point.y << " " << point.z << "\n";
    std::cerr << "direction: " << direction.x << " " << direction.y << " " << direction.z << "\n";
    std::cerr << "up: " << up.x << " " << up.y << " " << up.z << "\n";
    std::cerr << "aov: " << aov << "\n";
    std::cerr << "size: " << size.x << " " << size.y << "\n";
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

    QJsonArray pointJson;
    pointJson.push_back(point.x);
    pointJson.push_back(point.y);
    pointJson.push_back(point.z);
    modelJson["viewpoint"] = pointJson;

    QJsonArray directionJson;
    directionJson.push_back(direction.x);
    directionJson.push_back(direction.y);
    directionJson.push_back(direction.z);
    modelJson["viewdirection"] = directionJson;

    QJsonArray upJson;
    upJson.push_back(up.x);
    upJson.push_back(up.y);
    upJson.push_back(up.z);
    modelJson["viewup"] = upJson;

    QJsonArray sizeJson;
    sizeJson.push_back(size.x);
    sizeJson.push_back(size.y);
    modelJson["dimensions"] = sizeJson;

    modelJson["aov"] = aov;

    return modelJson;
  }

  virtual void restore(QJsonObject const& p) 
  {
    point.x = p["viewpoint"].toArray()[0].toDouble();
    point.y = p["viewpoint"].toArray()[1].toDouble();
    point.z = p["viewpoint"].toArray()[2].toDouble();

    direction.x = p["viewdirection"].toArray()[0].toDouble();
    direction.y = p["viewdirection"].toArray()[1].toDouble();
    direction.z = p["viewdirection"].toArray()[2].toDouble();

    up.x = p["viewup"].toArray()[0].toDouble();
    up.y = p["viewup"].toArray()[1].toDouble();
    up.z = p["viewup"].toArray()[2].toDouble();

    size.x = p["dimensions"].toArray()[0].toInt();
    size.y = p["dimensions"].toArray()[1].toInt();

    aov = p["aov"].toDouble();
  }

protected:
  gxy::vec3f point;
  gxy::vec3f direction;
  gxy::vec3f up;
  gxy::vec2i size;
  float aov;
};
