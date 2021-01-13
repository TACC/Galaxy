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

#include <iostream>

#include "IntelDevice.h"
#include "VklVolume.h"

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(IntelDevice)

void IntelDevice::InitDevice()
{
  IntelDevicePtr idp = IntelDevice::New();
  Device::SetDevice(idp);
}

static void embreeError(void *ptr, enum RTCError error, const char* msg)
{
  std::cerr << "Embree error: " << error << " :: " << msg << "\n";
}

static void vklError(void *ptr, VKLError error, const char* msg)
{
  std::cerr << "Vkl error: " << error << " :: " << msg << "\n";
}

IntelDevice::~IntelDevice()
{
  rtcReleaseDevice(embree);
  vklShutdown();
}

void
IntelDevice::initialize()
{
    super::initialize();

    embree = rtcNewDevice(NULL);
    rtcSetDeviceErrorFunction(embree, embreeError, (void *)this);

    vkl = vklNewDriver("ispc");
    vklDriverSetErrorCallback(vkl, vklError, (void *)this);
}

GalaxyObjectPtr
IntelDevice::CreateTheDeviceEquivalent(KeyedDataObjectPtr kdop)
{
  if (Volume::IsA(kdop))
  {
    VklVolumePtr v = VklVolume::New();
    v->SetVolume(Volume::Cast(kdop));
    return GalaxyObject::Cast(v);
  }
  else
    return NULL;
}

}
