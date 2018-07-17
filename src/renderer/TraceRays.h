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
#include <string.h>
#include <string>
#include <vector>

#include "dtypes.h"
#include "ISPCObject.h"
#include "Lighting.h"
#include "Rays.h"
#include "Visualization.h"

namespace gxy
{
KEYED_OBJECT_POINTER(TraceRays)

class TraceRays : public ISPCObject
{
public:
  TraceRays();
  ~TraceRays();

  virtual void LoadStateFromValue(rapidjson::Value&);
  virtual void SaveStateToValue(rapidjson::Value&, rapidjson::Document&);

  void SetEpsilon(float e);
  float GetEpsilon();

  RayList *Trace(Lighting*, VisualizationP, RayList *);

  virtual int SerialSize();
  virtual unsigned char *Serialize(unsigned char *);
  virtual unsigned char *Deserialize(unsigned char *);

protected:

  virtual void allocate_ispc();
  virtual void initialize_ispc();

};

} // namespace gxy
