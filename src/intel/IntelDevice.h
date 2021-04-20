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
#include "DeviceModel.h"

#include <embree3/rtcore.h>
#include <openvkl/openvkl.h>

namespace gxy 
{

class IntelDevice;
typedef std::shared_ptr<IntelDevice> IntelDevicePtr;


class IntelDevice : public Device
{
public:
  virtual ~IntelDevice();

  virtual void *GetTheDeviceEquivalent() { return (void *)&intel_device; }

  virtual void CreateTheDatasetDeviceEquivalent(KeyedDataObjectPtr);

  virtual DeviceModelPtr NewDeviceModel();
  
  RTCDevice   get_embree() { return intel_device.embree; }
  VKLDriver   get_vkl() { return intel_device.vkl; }

  static IntelDevicePtr Cast(DevicePtr d) { return std::dynamic_pointer_cast<IntelDevice>(d); }
  static void InitDevice()
  {
    IntelDevicePtr a = std::shared_ptr<IntelDevice>(new IntelDevice());
    Device::SetTheDevice(a);
  }

protected:
  IntelDevice();

  struct {
    RTCDevice   embree;
    VKLDriver   vkl;
  } intel_device;
};

}
