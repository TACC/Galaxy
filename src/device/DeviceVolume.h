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

namespace gxy { OBJECT_POINTER_TYPES(DeviceVolume); }

#include "Geometry.h"
#include "GeometryVis.h"
#include "Rays.h"
#include "Box.h"

#include <vector>

namespace gxy 
{

class DeviceVolume : public GalaxyObject
{
    GALAXY_OBJECT_SUBCLASS(DeviceVolume, GalaxyObject)

public:
    virtual void initialize();
    virtual ~DeviceVolume();

    void SetBoxes(Box g, Box l) { gbox = g; lbox = l; }

    void *GetDeviceEquivalent() { return device_equivalent; }

protected:
    virtual void allocate_device_equivalent();
    virtual void initialize_device_equivalent();
    virtual void delete_device_equivalent();

    void *device_equivalent = NULL;

    Box gbox;
    Box lbox;
};

}
