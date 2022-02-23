// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#undef NDEBUG

#include "Application.h"
#include <fcntl.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace gxy;

// ospray
#include "DataDrivenSpheres.h"
#include "common/Data.h"
#include "common/Model.h"
#include "common/OSPCommon.h"
#include "transferFunction/TransferFunction.h"

// ispc-generated files
#include "DataDrivenSpheres_ispc.h"

namespace gxy { void DDSpheres_Hello() { std::cerr << "hello\n"; } }

static int frame_count = 0;

namespace ospray {

  DataDrivenSpheres::DataDrivenSpheres()
  {
    this->ispcEquivalent = ispc::DataDrivenSpheres_create(this);
  }

  DataDrivenSpheres::~DataDrivenSpheres()
  {
  }

  std::string DataDrivenSpheres::toString() const
  {
    return "ospray::DataDrivenSpheres";
  }

  void DataDrivenSpheres::finalize(Model *model)
  {
    Geometry::finalize(model);

    centers      = getParamData("centers");
    data         = getParamData("data");

    radius0 = getParam1f("radius0", 0.1);
    radius1 = getParam1f("radius1", 0.0);

    value0 = getParam1f("value0", 0.0);
    value1 = getParam1f("value1", 0.0);

    numDataDrivenSpheres = centers->numItems;

    postStatusMsg(2) << "#galaxy: creating 'ddspheres' geometry, #ddspheres = "
                     << numDataDrivenSpheres;

    auto transferFunction = (TransferFunction *)getParamData("transferFunction", nullptr);

    float box[6];
    ispc::DataDrivenSpheresGeometry_set(getIE(),
                              model->getIE(),
                              centers->data,
                              data->data,
                              numDataDrivenSpheres,
                              radius0,
                              radius1,
                              value0,
                              value1,
                              transferFunction->getIE(),
                              box);

    bounds.lower.x = box[0];
    bounds.lower.y = box[1];
    bounds.lower.z = box[2];

    bounds.upper.x = box[3];
    bounds.upper.y = box[4];
    bounds.upper.z = box[5];
  }

  OSP_REGISTER_GEOMETRY(DataDrivenSpheres,ddspheres);

} // ::ospray
