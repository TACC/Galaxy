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

// #define OSPRAY_SDK_INTERFACE 
// #define OSPRAY_MODULE_ISPC_INTERFACE

#pragma once

#include "ospray/SDK/geometry/Geometry.h"
#include "ospray/OSPDataType.h"

namespace ospray {
  /*! @{ \defgroup geometry_psurface PointSurface ("psurface")

    \ingroup ospray_supported_geometries

    \brief Geometry representing spheres with a radius dependent on per-sphere data

    Implements a geometry consisting of individual spheres, each of
    which can has a data value that is passed against a transfer function
    to produce a radius.

    Parameters:
    <dl>
    <dt><code>Data<vec3f>  points</code></dt><dd> Array of points.</dd>
    </dl>

    The functionality for this geometry is implemented via the
    \ref ospray::PointSurface class.

  */

  /*! \brief A geometry for a set of psurface

    Implements the \ref geometry_spheres geometry

  */
  struct OSPRAY_SDK_INTERFACE PointSurface : public Geometry
  {
    PointSurface();
    ~PointSurface();

    virtual std::string toString() const override;
    virtual void finalize(Model *model) override;

    // Data members //

    size_t numPoints;
    Ref<Data> points;
    float epsilon;
  };
  /*! @} */

} // ::ospray

