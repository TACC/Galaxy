// ========================================================================== //
// Copyright (c) 2014-2020 The University of Texas at Austin.               //
// All rights reserved.                                                     //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// A copy of the License is included with this software in the file LICENSE.  //
// If your copy does not contain the License, you may obtain a copy of the  //
// License at:                                                              //
//                                                                          //
//   https://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  //
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.         //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
//                                                                          //
// ========================================================================== //

#pragma once

#include "Device.h"
#include "Model.h"

#include <embree3/rtcore.h>
#include <openvkl/openvkl.h>

namespace gxy 
{

OBJECT_POINTER_TYPES(IntelDevice)

class IntelDevice : public Device
{
  GALAXY_OBJECT_SUBCLASS(IntelDevice, Device)

public:
  virtual ~IntelDevice();

  static  void InitDevice();
  virtual void initialize();

  RTCDevice get_embree();
  virtual ModelPtr NewModel();

  virtual GalaxyObjectPtr CreateTheDeviceEquivalent(KeyedDataObjectPtr);

protected:

  RTCDevice   embree;
  VKLDriver   vkl;
};

}





  


