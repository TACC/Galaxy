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

#include "DeviceModel.h"

namespace gxy
{

OBJECT_CLASS_TYPE(DeviceModel)

DeviceModel::~DeviceModel()
{
  if (device_equivalent) 
    free(device_equivalent);
  device_equivalent = NULL;
}

void
DeviceModel::initialize()
{
    super::initialize();
}

int
DeviceModel::AddGeometry(GeometryDPtr g, GeometryVisDPtr vis)
{
    int id;

    if (free_geometry_ids.size())
    {
        id = free_geometry_ids.back();
        free_geometry_ids.pop_back();
        geometries[id] = g;
        geometry_vis[id] = vis;
    }
    else
    {
        id = geometries.size();
        geometries.push_back(g);
        geometry_vis.push_back(vis);
    }

    return id;
}

int
DeviceModel::RemoveGeometry(GeometryDPtr g)
{
    for (int id = 0; id < geometries.size() ; id++)
        if (geometries[id] == g)
        {
            geometries[id] = NULL;
            geometry_vis[id] = NULL;
            free_geometry_ids.push_back(id);
            return id;
        }

    std::cerr << "DeviceModel::RemoveGeometry: geometry not found\n";
    return -1;
}

void
DeviceModel::RemoveGeometry(int id)
{
    if (geometries[id])
    {
        geometries[id] = NULL;
        geometry_vis[id] = NULL;
        free_geometry_ids.push_back(id);
    }
}

void DeviceModel::Intersect(RayList *r) {}

void DeviceModel::Build() {}

}
