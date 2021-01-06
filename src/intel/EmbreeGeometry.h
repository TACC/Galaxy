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

#include "GalaxyObject.h"

namespace gxy { OBJECT_POINTER_TYPES(EmbreeGeometry) }

#include "Geometry.h"
#include "Embree.h"
#include "EmbreeGeometry_ispc.h"

#include <embree3/rtcore.h>

namespace gxy 
{

class EmbreeGeometry
{
public:
    static EmbreeGeometryDPtr New() { return std::shared_ptr<EmbreeGeometry>(new EmbreeGeometry); }
    virtual ~EmbreeGeometry();

    void SetGeometry(GeometryDPtr g);

    virtual void CreateIspc();
    virtual void FinalizeIspc();

    GeometryDPtr GetGeometry();
    RTCGeometry GetDeviceGeometry() { return device_geometry; }
    void *GetIspc() { return ispc; }

protected:
    EmbreeGeometry();

    GeometryDPtr geometry;
    RTCGeometry device_geometry;
    void *ispc;
};

}
    
