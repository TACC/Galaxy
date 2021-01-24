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

#include "IntelData.h"

namespace gxy { OBJECT_POINTER_TYPES(EmbreeGeometry) }

#include "Geometry.h"
#include "EmbreeGeometry_ispc.h"

#include <embree3/rtcore.h>

namespace gxy 
{

class EmbreeGeometry : public IntelData
{
    GALAXY_OBJECT_SUBCLASS(EmbreeGeometry, IntelData)

public:
    virtual ~EmbreeGeometry();
    virtual void initialize();

    virtual void FinalizeData(KeyedDataObjectPtr);

    RTCGeometry GetDeviceGeometry()
    {
      return (RTCGeometry)((::ispc::EmbreeGeometry_ispc *)GetIspc())->device_geometry;
    }

    void SetDeviceGeometry(RTCGeometry dg)
    {
      ((::ispc::EmbreeGeometry_ispc *)GetIspc())->device_geometry = (void *)dg;
    }

protected:
    virtual int  IspcSize();
};

}
    
