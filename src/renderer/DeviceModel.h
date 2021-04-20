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

#pragma once

#include "KeyedObject.h"

namespace gxy { OBJECT_POINTER_TYPES(DeviceModel); }

#include "Geometry.h"
#include "GeometryVis.h"
#include "Rays.h"
#include "Box.h"

#include <vector>

namespace gxy 
{

class DeviceModel : public GalaxyObject
{
    GALAXY_OBJECT_SUBCLASS(DeviceModel, GalaxyObject)

public:
    virtual void initialize();
    virtual ~DeviceModel();
    
    virtual int  AddGeometry(GeometryDPtr, GeometryVisDPtr);
    virtual void RemoveGeometry(int);
    virtual int  RemoveGeometry(GeometryDPtr);


    virtual void Build();
    
    virtual void Intersect(RayList *);

    void SetBoxes(Box g, Box l) { gbox = g; lbox = l; }

    void *GetDeviceEquivalent() { return device_equivalent; }

protected:
    void *device_equivalent = NULL;

    std::vector<GeometryDPtr> geometries;
    std::vector<GeometryVisDPtr> geometry_vis;
    std::vector<int> free_geometry_ids;

    Box gbox;
    Box lbox;
};

}
