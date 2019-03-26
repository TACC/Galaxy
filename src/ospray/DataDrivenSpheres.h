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

#include "Geometry.h"
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
    <dt><code>float        radius = 0.01f</code></dt><dd>Base radius common to all spheres if 'offset_datavalue' is not used</dd>
    <dt><code>int32        materialID = 0</code></dt><dd>Material ID common to all spheres if 'offset_materialID' is not used</dd>
    <dt><code>int32        bytes_per_sphere = 4*sizeof(float)</code></dt><dd>Size (in bytes) of each sphere in the data array.</dd>
    <dt><code>int32        offset_center = 0</code></dt><dd>Offset (in bytes) of each sphere's 'vec3f center' value within the sphere</dd>
    <dt><code>int32        offset_datavalue = -1</code></dt><dd>Offset (in bytes) of each sphere's 'float data' value within each sphere. Setting this value to -1 means that there is no per-sphere data value, and that all spheres should use the (shared) 'radius' value instead</dd>
    <dt><code>int32        offset_materialID = -1</code></dt><dd>Offset (in bytes) of each sphere's 'int materialID' value within each sphere. Setting this value to -1 means that there is no per-sphere material ID, and that all spheres share the same per-geometry 'materialID'</dd>
    <dt><code>Data<float>  spheres</code></dt><dd> Array of data elements.</dd>

    <dt><code>Radius0<float>  R0</code></dt><dd> Radius 0.</dd>
    <dt><code>Radius1<float>  R1</code></dt><dd> Radius 1.</dd>
    <dt><code>Value0<float>    V0</code></dt><dd> Data value 0.</dd>
    <dt><code>Value1<float>    V1</code></dt><dd> Data value 1.</dd>
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

    /*! default radius, if no per-sphere radius was specified. */
    float radius;
    int32 materialID;

    size_t numDataDrivenSpheres;
    size_t bytesPerSphere; //!< num bytes per sphere
    int64 offset_center;
    int64 offset_datavalue;
    int64 offset_materialID;
    int64 offset_colorID;

    float radius0;
    float radius1;
    float value0;
    float value1;

    Ref<Data> sphereData;

    /*! data array from which we read the per-sphere color data; if
      NULL we do not have per-sphere data */
    Ref<Data> colorData;

    Ref<Data> texcoordData;

    /*! The color format of the colorData array, one of:
        OSP_FLOAT3, OSP_FLOAT3A, OSP_FLOAT4 or OSP_UCHAR4 */
    OSPDataType colorFormat;

    /*! stride in colorData array for accessing i'th sphere's
      color. color of sphere i will be read as colorFormat color from
      'colorOffset+i*colorStride */
    size_t colorStride;

    /*! offset in colorData array for accessing i'th sphere's
      color. color of sphere i will be read as colorFormat color from
      'colorOffset+i*colorStride */
    size_t colorOffset;

    float epsilon;  //epsilon for intersections
  };
  /*! @} */

} // ::ospray

