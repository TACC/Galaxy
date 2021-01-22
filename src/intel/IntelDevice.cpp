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
#include "IntelModel.h"

#include "VklVolume.h"

#include "EmbreePathLines.h"
#include "EmbreeSpheres.h"
#include "EmbreeTriangles.h"
#include "EmbreeGeometry.h"

#include <embree3/rtcore.h>
#include <openvkl/openvkl.h>


namespace gxy
{

static void embreeError(void *ptr, enum RTCError error, const char* msg)
{
  std::cerr << "Embree error: " << error << " :: " << msg << "\n";
}

static void vklError(void *ptr, VKLError error, const char* msg)
{
  std::cerr << "Vkl error: " << error << " :: " << msg << "\n";
}

IntelDevice::IntelDevice()
{
  std::cerr << "XXXX\n";

  EmbreeGeometry::RegisterClass();
  EmbreeTriangles::RegisterClass();
  EmbreePathLines::RegisterClass();
  EmbreeSpheres::RegisterClass();
  VklVolume::RegisterClass();

  intel_device.embree = rtcNewDevice(NULL);
  rtcSetDeviceErrorFunction(intel_device.embree, embreeError, (void *)this);

  vklLoadModule("ispc_driver");
  intel_device.vkl = vklNewDriver("ispc");
  vklDriverSetErrorCallback(intel_device.vkl, vklError, (void *)this);
}

IntelDevice::~IntelDevice()
{
  rtcReleaseDevice(intel_device.embree);
  vklShutdown();
}

ModelPtr
IntelDevice::NewModel() 
{
  return Model::Cast(IntelModel::New());
}

void
IntelDevice::CreateTheDatasetDeviceEquivalent(KeyedDataObjectPtr kdop)
{
  IntelDataPtr idata;

  if (Volume::IsA(kdop))
    idata = IntelData::Cast(VklVolume::New());
  else if (Particles::IsA(kdop))
    idata = IntelData::Cast(EmbreeSpheres::New());
  else if (PathLines::IsA(kdop))
    idata = IntelData::Cast(EmbreePathLines::New());
  else if (Triangles::IsA(kdop))
    idata = IntelData::Cast(EmbreeTriangles::New());
  else
  {
    std::cerr << "error - unknown dataset class\n";
    exit(1);
  }

  idata->FinalizeData(kdop);
  kdop->SetTheDeviceEquivalent(idata);
}

}
