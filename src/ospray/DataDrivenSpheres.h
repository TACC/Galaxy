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
  /*! @{ \defgroup geometry_ddspheres DataDrivenSpheres ("ddspheres")

    \ingroup ospray_supported_geometries

    \brief Geometry representing spheres with a radius dependent on per-sphere data

    Implements a geometry consisting of individual spheres, each of
    which can has a data value that is passed against a transfer function
    to produce a radius.

    Parameters:
    <dl>
    <dt><code>Data<vec3f>  centers</code></dt><dd> Array of sphere centers.</dd>
    <dt><code>Data<float>  data</code></dt><dd> Array of per-sphere data values.</dd>

    <dt><code>float radius0 = 0.1 </code></dt><dd> Radius at data value 0.</dd>
    <dt><code>float radius1 = 0.0 </code></dt><dd> Radius at data value 1.</dd>
    <dt><code>float value0  = 0.0 </code></dt><dd> Data value 0.</dd>
    <dt><code>float value1  = 0.0 </code></dt><dd> Data value 1.</dd>
    </dl>

    The functionality for this geometry is implemented via the
    \ref ospray::DataDrivenSpheres class.

  */

  /*! \brief A geometry for a set of ddspheres

    Implements the \ref geometry_spheres geometry

  */
  struct OSPRAY_SDK_INTERFACE DataDrivenSpheres : public Geometry
  {
    DataDrivenSpheres();
    ~DataDrivenSpheres();

    virtual std::string toString() const override;
    virtual void finalize(Model *model) override;

    // Data members //

    size_t numDataDrivenSpheres;

    float radius0;
    float radius1;
    float value0;
    float value1;

    Ref<Data> centers;
    Ref<Data> data;

    float epsilon;  //epsilon for intersections
  };
  /*! @} */

} // ::ospray

