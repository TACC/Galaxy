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

#include "Application.h"
#include "IntelDevice.h"
#include "IntelModel.h"
#include "Rays.h"

#include "IntelModel_ispc.h"

// #include <embree3/rtcore_scene.h>
// #include <openvkl/openvkl.isph>

namespace gxy
{

OBJECT_CLASS_TYPE(IntelModel)

void
IntelModel::initialize()
{
  super::initialize();
  ispc.scene = NULL;
}

IntelModel::~IntelModel()
{
  if (ispc.scene)
    rtcReleaseScene(ispc.scene);

  ispc.scene = NULL;
  if (ispc.geometries) free((void *)ispc.geometries);
  if (ispc.volumes) free((void *)ispc.volumes);
}

void
IntelModel::Build()
{
  IntelDevicePtr idev = IntelDevice::Cast(Device::GetTheDevice());
  ispc.scene = rtcNewScene(idev->get_embree());

  if (geometries.size())
  {
    ispc.nGeometries = geometries.size();
    ispc.geometries = (::ispc::EmbreeGeometry_ispc **)malloc(geometries.size() * sizeof(::ispc::EmbreeGeometry_ispc *));
    ispc.geometry_vis = (::ispc::Vis_ispc **)malloc(geometries.size() * sizeof(::ispc::Vis_ispc *));
  }
  else
  {
    ispc.nGeometries = 0;
    ispc.geometries = NULL;
    ispc.geometry_vis = NULL;
  }

  if (volumes.size())
  {
    ispc.nVolumes = volumes.size();
    ispc.volumes = (::ispc::VklVolume_ispc **)malloc(volumes.size() * sizeof(::ispc::VklVolume_ispc *));
    ispc.volume_vis = (::ispc::Vis_ispc **)malloc(volumes.size() * sizeof(::ispc::Vis_ispc *));
  }
  else
  {
    ispc.nVolumes = 0;
    ispc.volumes = NULL;
    ispc.volume_vis = NULL;
  }

  int i = 0;
  for (auto g : geometries)
  {
    EmbreeGeometryPtr eg = EmbreeGeometry::Cast(g->GetTheDeviceEquivalent());
    if (! eg)
    {
      std::cerr << "IntelModel::Build : EmbreeGeometry has no device equivalent\n";    
      exit(1);
    }

    rtcAttachGeometryByID(ispc.scene, eg->GetDeviceGeometry(), i);

    ispc.geometries[i] = (::ispc::EmbreeGeometry_ispc *)eg->GetIspc();
    ispc.geometry_vis[i] = (::ispc::Vis_ispc *)geometry_vis[i]->GetIspc();
    i++;
  }

  i = 0;
  for (auto v : volumes)
  {
    VklVolumePtr ev = VklVolume::Cast(v->GetTheDeviceEquivalent());
    if (! ev)
    {
      std::cerr << "IntelModel::Build : VklVolume has no device equivalent\n";    
      exit(1);
    }

    ispc.volumes[i] = (::ispc::VklVolume_ispc *)ev->GetIspc();
    ispc.volume_vis[i] = (::ispc::Vis_ispc *)volume_vis[i]->GetIspc();
    i++;
  }

  rtcCommitScene(ispc.scene);
}

void
IntelModel::Intersect(RayList *rays)
{
  ::ispc::IntelModel_Intersect(ispc, rays->GetRayCount(), rays->GetIspc());
}

void
IntelModel::IsoCrossing(RayList *rays, int n, float *values)
{
  ::ispc::IntelModel_IsoCrossing(ispc, rays->GetRayCount(), rays->GetIspc(), n, values);
}

void
IntelModel::Sample(int n, float *x, float *y, float *z, float *d)
{
  ::ispc::IntelModel_Sample(ispc, n, x, y, z, d);
}

}
