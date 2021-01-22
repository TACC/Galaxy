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
namespace gxy { OBJECT_POINTER_TYPES(EmbreePathLines) }

#include <vector>

#include "EmbreeGeometry.h"
#include "PathLines.h"
#include "EmbreePathLines_ispc.h"

namespace gxy 
{

class EmbreePathLines : public EmbreeGeometry
{
    GALAXY_OBJECT_SUBCLASS(EmbreePathLines, EmbreeGeometry);

public:
    virtual void initialize();
    ~EmbreePathLines() {};

    void SetMap(float, float, float, float);

    virtual void FinalizeData(KeyedDataObjectPtr);

protected:
    virtual int IspcSize() { return sizeof(::ispc::EmbreePathLines_ispc); }

private:
    std::vector<vec4f> vCurve;
    std::vector<int>   iCurve;
};

}
