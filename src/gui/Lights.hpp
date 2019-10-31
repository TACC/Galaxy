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

  void set_type(int t)       { type = t; }

protected:
  gxy::vec3f point;
  int type;
};