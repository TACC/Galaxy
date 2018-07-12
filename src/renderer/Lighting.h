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

#include <iostream>
#include <vector>

#include "dtypes.h"
#include "rapidjson/document.h"
#include "ISPCObject.h"

namespace gxy
{
class Lighting : public ISPCObject
{
public:
   Lighting();
  ~Lighting();

  virtual int SerialSize();
  virtual unsigned char *Serialize(unsigned char *);
  virtual unsigned char *Deserialize(unsigned char *);

  void LoadStateFromValue(rapidjson::Value&);
  void SaveStateToValue(rapidjson::Value&, rapidjson::Document&);

  void SetLights(int n, float* l);
  void SetLights(int n, vec3f* l) { SetLights(n, (float *)l); }
  void GetLights(int& n, float*& l);

  void SetK(float ka, float kd);
  void GetK(float& ka, float& kd);

  void SetAO(int n, float r);
  void GetAO(int& n, float& r);

  void SetShadowFlag(bool b);
  void GetShadowFlag(bool& b);

  void SetEpsilon(float e);
  void GetEpsilon(float& e);

protected:
  virtual void allocate_ispc();
  virtual void initialize_ispc();
};
}
