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
#include "DataDrivenPathLines.h"
#include "common/Data.h"
#include "common/Model.h"
#include "ospcommon/utility/DataView.h"
#include "transferFunction/TransferFunction.h"

// ispc-generated files
#include "DataDrivenPathLines_ispc.h"

#define MAP_RADIUS(d, r)                                                                  \
{                                                                                         \
  if (value0 == value1) r = radius0;                                                      \
  else                                                                                    \
  {                                                                                       \
    float R = (d - value0) / (value1 - value0);                                           \
    r = (R <= 0.0) ? radius0 : (R >= 1.0) ? radius1 : radius0 + R*(radius1 - radius0);    \
  }                                                                                       \
}

namespace ospray {

  DataDrivenPathLines::DataDrivenPathLines()
  {
    std::cerr << "DataDrivenPathLines::DataDrivenPathLines entry... calling create\n";
    this->ispcEquivalent = ispc::DataDrivenPathLines_create(this);
    std::cerr << "DataDrivenPathLines::DataDrivenPathLines entry... create return\n";
  }

  std::string DataDrivenPathLines::toString() const
  {
    return "ospray::DataDrivenPathLines";
  }

  void DataDrivenPathLines::finalize(Model *model)
  {
    Geometry::finalize(model);

    const float globalRadius = getParam1f("radius",0.01f);

    utility::DataView<const float> radius(&globalRadius, 0);

    radius0 = getParam1f("radius0", globalRadius);
    radius1 = getParam1f("radius1", 0.0);
    value0 = getParam1f("value0", 0.0);
    value1 = getParam1f("value1", 0.0);

    vertexData = getParamData("vertices",nullptr);
    if (! vertexData || vertexData->type != OSP_FLOAT3)
      throw std::runtime_error("ddpathlines missing or invalid vertices");

    vertices = (vec3f*)vertexData->data;
    numVertices = vertexData->numItems;

    dataData = getParamData("data",nullptr);
    if (! dataData || dataData->type != OSP_FLOAT)
      throw std::runtime_error("ddpathlines missing or invalid vertex data");

    data = (float *)dataData->data;

    indexData  = getParamData("indices",nullptr);
    if (! indexData || indexData->type != OSP_INT)
      throw std::runtime_error("ddpathlines must have 'index' array");

    if (indexData->type != OSP_INT)
      throw std::runtime_error("ddpathlines 'index' array must be type OSP_INT");

    indices = (uint32*)indexData->data;
    numSegments = indexData->numItems;

    postStatusMsg(2) << "#osp: creating ddpathlines geometry, "
                     << "#verts=" << numVertices << ", "
                     << "#segments=" << numSegments << "\n";

    bounds = empty;

    // XXX curves may actually have a larger bounding box due to swinging

    for (uint32_t i = 0; i < numVertices; i++)
    {
      float radius;
      MAP_RADIUS(data[i], radius);
      bounds.extend(vertices[i] - radius);
      bounds.extend(vertices[i] + radius);
    }

    vertexCurve.clear();
    indexCurve.clear();

    bool middleSegment = false;
    vec3f tangent;
    int pline = 0;

    for (uint32_t i = 0; i < numSegments; i++)
    {
      const uint32 idx = indices[i];
      const vec3f start = vertices[idx];
      const vec3f end = vertices[idx+1];

      const float lengthSegment = length(start - end);

      float startRadius, endRadius;

      MAP_RADIUS (data[idx], startRadius);
      MAP_RADIUS (data[idx+1], endRadius);

      // startRadius = data[idx];
      // endRadius = data[idx+1];

      indexCurve.push_back(vertexCurve.size());

      if (middleSegment)
      {
        vertexCurve.push_back(vec4f(start, startRadius));
        vertexCurve.push_back(vec4f(start + tangent, lerp(1.f/3, startRadius, endRadius)));
      }
      else
      { // start new curve
        vertexCurve.push_back(vec4f(start, startRadius));
        vertexCurve.push_back(vec4f(start, startRadius));
      }

      middleSegment = i+1 < numSegments && indices[i+1] == idx+1;

      if (middleSegment)
      {
        const vec3f next = vertices[idx+2];
        const vec3f delta = (1.f/3)*(next - start);
        const float b = length(next - end);
        const float r = lengthSegment/(lengthSegment + b);

        vertexCurve.push_back(vec4f(end - r*delta, lerp(2.f/3, startRadius, endRadius)));

        tangent = (1.f-r)*delta;
      }
      else { // end curve
        vertexCurve.push_back(vec4f(end, endRadius));
        vertexCurve.push_back(vec4f(end, endRadius));
      }
    }

    auto transferFunction = (TransferFunction *)getParamData("transferFunction", nullptr);
    if (! transferFunction) std::cerr << "no TF\n";

    float *vrt = (float *)vertexCurve.data();
    int   *idx = (int *)indexCurve.data();
  
    ispc::DataDrivenPathLines_setCurve(getIE(),model->getIE(),
        (const float *)vertexCurve.data(), vertexCurve.size(),
        indexCurve.data(), indexCurve.size(),
        transferFunction->getIE(), radius0, radius1, value0, value1);
  }

  OSP_REGISTER_GEOMETRY(DataDrivenPathLines,ddpathlines);

} // ::ospray
