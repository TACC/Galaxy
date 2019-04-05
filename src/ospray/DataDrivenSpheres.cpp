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
#include "DataDrivenSpheres.h"
#include "common/Data.h"
#include "common/Model.h"
#include "common/OSPCommon.h"
#include "transferFunction/TransferFunction.h"

// ispc-generated files
#include "DataDrivenSpheres_ispc.h"

namespace gxy { void DDSpheres_Hello() { std::cerr << "hello\n"; } }

namespace ospray {

  DataDrivenSpheres::DataDrivenSpheres()
  {
    this->ispcEquivalent = ispc::DataDrivenSpheres_create(this);
  }

  DataDrivenSpheres::~DataDrivenSpheres()
  {
    if (getIE()) ispc::DataDrivenSpheres_freeMappedRadius(getIE());
  }

  std::string DataDrivenSpheres::toString() const
  {
    return "ospray::DataDrivenSpheres";
  }

  void DataDrivenSpheres::finalize(Model *model)
  {
    Geometry::finalize(model);

    materialID        = getParam1i("materialID",0);
    bytesPerSphere    = getParam1i("bytes_per_sphere",4*sizeof(float));
    texcoordData      = getParamData("texcoord");
    offset_center     = getParam1i("offset_center",0);
    offset_datavalue  = getParam1i("offset_datavalue",-1);
    offset_materialID = getParam1i("offset_materialID",-1);
    offset_colorID    = getParam1i("offset_colorID",-1);
    sphereData        = getParamData("spheres");
    colorData         = getParamData("color");
    colorOffset       = getParam1i("color_offset",0);

    radius0 = getParam1f("radius0", 0.1);
    radius1 = getParam1f("radius1", 0.0);

    value0 = getParam1f("value0", 0.0);
    value1 = getParam1f("value1", 0.0);

    if (colorData) {
      if (hasParam("color_format")) {
        colorFormat = static_cast<OSPDataType>(getParam1i("color_format",
                                                          OSP_UNKNOWN));
      } else {
        colorFormat = colorData->type;
      }
      if (colorFormat != OSP_FLOAT4 && colorFormat != OSP_FLOAT3
          && colorFormat != OSP_FLOAT3A && colorFormat != OSP_UCHAR4)
      {
        throw std::runtime_error("Galaxy DataDrivenSpheres: invalid "
                                 "colorFormat specified! Must be one of: "
                                 "OSP_FLOAT4, OSP_FLOAT3, OSP_FLOAT3A or "
                                 "OSP_UCHAR4.");
      }
    } else {
      colorFormat = OSP_UNKNOWN;
    }
    colorStride = getParam1i("color_stride",
                             colorFormat == OSP_UNKNOWN ?
                             0 : sizeOf(colorFormat));


    if (sphereData.ptr == nullptr) {
      throw std::runtime_error("Galaxy DataDrivenSpheres: no 'spheres' data "
                               "specified");
    }

    numDataDrivenSpheres = sphereData->numBytes / bytesPerSphere;
    postStatusMsg(2) << "#galaxy: creating 'ddspheres' geometry, #ddspheres = "
                     << numDataDrivenSpheres;

    if (numDataDrivenSpheres >= (1ULL << 30)) {
      throw std::runtime_error("#ospray::DataDrivenSpheres: too many spheres in this "
                               "sphere geometry. Consider splitting this "
                               "geometry in multiple geometries with fewer "
                               "spheres (you can still put all those "
                               "geometries into a single model, but you can't "
                               "put that many spheres into a single geometry "
                               "without causing address overflows)");
    }

    // check whether we need 64-bit addressing
    bool huge_mesh = false;
    if (colorData && colorData->numBytes > INT32_MAX)
      huge_mesh = true;
    if (texcoordData && texcoordData->numBytes > INT32_MAX)
      huge_mesh = true;

    // std::cerr << "About to call _set\n";

    auto transferFunction = (TransferFunction *)getParamData("transferFunction", nullptr);

    ispc::DataDrivenSpheresGeometry_set(getIE(),
                              model->getIE(),
                              sphereData->data,
                              materialList ? ispcMaterialPtrs.data() : nullptr,
                              texcoordData ?
                                  (ispc::vec2f *)texcoordData->data : nullptr,
                              colorData ? colorData->data : nullptr,
                              colorOffset,
                              colorStride,
                              colorFormat,
                              numDataDrivenSpheres,
                              bytesPerSphere,
                              materialID,
                              offset_center,
                              offset_datavalue,
                              offset_materialID,
                              offset_colorID,
                              huge_mesh,
                              radius0,
                              radius1,
                              value0,
                              value1,
                              transferFunction->getIE());


    // std::cerr << "About to compute radii\n";
    if (value0 != value1)
      ispc::DataDrivenSpheresGeometry_computeRadius(getIE(), &bounds);
  }

  OSP_REGISTER_GEOMETRY(DataDrivenSpheres,ddspheres);

} // ::ospray
