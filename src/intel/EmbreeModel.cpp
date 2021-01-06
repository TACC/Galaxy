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
#include "Embree.h"
#include "EmbreeModel.h"

#include <embree3/rtcore_scene.h>

#include "EmbreeModel_ispc.h"

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(EmbreeModel)

void
EmbreeModel::Register()
{
    RegisterClass();
}

void
EmbreeModel::initialize()
{
    super::initialize();
    embree = NULL;
    scene = NULL;
}

void
EmbreeModel::SetEmbree(EmbreeDPtr e)
{
    embree = e;
}

int
EmbreeModel::serialSize()
{
    return super::serialSize() + sizeof(Key);
}

unsigned char *
EmbreeModel::serialize(unsigned char *p)
{
    p = super::serialize(p);
    *(Key *)p = embree->getkey();
    p = p + sizeof(key);
    
    return p;
}

unsigned char *
EmbreeModel::deserialize(unsigned char *p)
{
    p = super::deserialize(p);
    embree = Embree::GetByKey(*(Key *)p);
    p = p + sizeof(key);

    return p;
}

bool
EmbreeModel::local_commit(MPI_Comm c)
{
    if (! scene)
        scene = rtcNewScene(embree->Device());
    
    rtcCommitScene(scene);

    return false;
}

EmbreeModel::~EmbreeModel()
{
    rtcReleaseScene(scene);
}

int
EmbreeModel::AddGeometry(EmbreeGeometryDPtr g)
{
    int id;

    if (free_ids.size())
    {
        id = free_ids.back();
        free_ids.pop_back();

        geometries[id] = g;
    }
    else
    {
        id = geometries.size();
        geometries.push_back(g);
    }

    rtcAttachGeometryByID(scene, g->GetDeviceGeometry(), id);

    return id;
}

void
EmbreeModel::RemoveGeometry(EmbreeGeometryDPtr g)
{
    for (int id = 0; id < geometries.size() ; id++)
        if (geometries[id] == g)
        {
            geometries[id] = NULL;
            rtcDetachGeometry(scene, id);
            free_ids.push_back(id);
            return;
        }

    std::cerr << "EmbreeModel::RemoveGeometry: geometry not found\n";
}

void
EmbreeModel::RemoveGeometry(int id)
{
    if (geometries[id])
    {
        geometries[id] = NULL;
        free_ids.push_back(id);
    }
}

void
EmbreeModel::Intersect(RayList *rays)
{
    void** geoms = new void*[geometries.size()];

    for (int i = 0; i < geometries.size(); i++)
        geoms[i] = geometries[i] ? geometries[i]->GetIspc() : NULL;

    ispc::EmbreeModel_Intersect(GetDevice(), geoms, rays->GetRayCount(), rays->GetIspc());

    delete[] geoms;
}

}
