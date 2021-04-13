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

#include "Model.h"

namespace gxy
{

OBJECT_CLASS_TYPE(Model)

Model::~Model()
{
  if (device_equivalent) 
    free(device_equivalent);
  device_equivalent = NULL;
}

void
Model::initialize()
{
    super::initialize();
}

int
Model::AddGeometry(GeometryDPtr g, GeometryVisDPtr vis)
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
Model::RemoveGeometry(GeometryDPtr g)
{
    for (int id = 0; id < geometries.size() ; id++)
        if (geometries[id] == g)
        {
            geometries[id] = NULL;
            geometry_vis[id] = NULL;
            free_geometry_ids.push_back(id);
            return id;
        }

    std::cerr << "Model::RemoveGeometry: geometry not found\n";
    return -1;
}

void
Model::RemoveGeometry(int id)
{
    if (geometries[id])
    {
        geometries[id] = NULL;
        geometry_vis[id] = NULL;
        free_geometry_ids.push_back(id);
    }
}

int
Model::AddVolume(VolumeDPtr v, VolumeVisDPtr vis)
{
    int id;

    if (free_volume_ids.size())
    {
        id = free_volume_ids.back();
        free_volume_ids.pop_back();
        volumes[id] = v;
        volume_vis[id] = vis;
    }
    else
    {
        id = volumes.size();
        volumes.push_back(v);
        volume_vis.push_back(vis);
    }

    return id;
}

int
Model::RemoveVolume(VolumeDPtr g)
{
    for (int id = 0; id < volumes.size() ; id++)
        if (volumes[id] == g)
        {
            volumes[id] = NULL;
            volume_vis[id] = NULL;
            free_volume_ids.push_back(id);
            return id;
        }

    std::cerr << "Model::RemoveVolume: volume not found\n";
    return -1;
}

void
Model::RemoveVolume(int id)
{
    if (id != -1 && volumes[id])
    {
        volumes[id] = NULL;
        volume_vis[id] = NULL;
        free_volume_ids.push_back(id);
    }

    std::cerr << "Model::RemoveVolume: volume not found\n";
}

void Model::Intersect(RayList *r) {}
void Model::IsoCrossing(RayList *r, int n, float *v) {}
void Model::Sample(int n, float* x, float* y, float* z, float* d) {}
void Model::Build() {}

}
