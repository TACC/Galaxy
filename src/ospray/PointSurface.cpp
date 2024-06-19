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

// ospray
#include "PointSurface.h"
#include "common/Data.h"
#include "common/Model.h"
#include "common/OSPCommon.h"
#include "transferFunction/TransferFunction.h"

// ispc-generated files
#include "PointSurface_ispc.h"

namespace gxy { void PSurface_Hello() { std::cerr << "hello\n"; } }

namespace ospray {

  PointSurface::PointSurface()
  {
    this->ispcEquivalent = ispc::PointSurface_create(this);
  }

  PointSurface::~PointSurface()
  {
  }

  std::string PointSurface::toString() const
  {
    return "ospray::PointSurface";
  }

  void PointSurface::finalize(Model *model)
  {
    Geometry::finalize(model);

    points    = getParamData("points");
    numPoints = points->numItems;

    postStatusMsg(2) << "#galaxy: creating 'psurface' geometry, #points = "
                     << numPoints << "\n";

    float box[6];
    ispc::PointSurfaceGeometry_set(getIE(),
                              model->getIE(),
                              points->data,
                              numPoints,
                              box);

    bounds.lower.x = box[0];
    bounds.lower.y = box[1];
    bounds.lower.z = box[2];

    bounds.upper.x = box[3];
    bounds.upper.y = box[4];
    bounds.upper.z = box[5];
  }

  OSP_REGISTER_GEOMETRY(PointSurface,psurface);

} // ::ospray
