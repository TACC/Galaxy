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

// ispc-generated files
#include "DataDrivenPathLines_ispc.h"

#define MAP_RADIUS(d, r)                                                                  \
{                                                                                         \
  if (value0 == value1) r = radius1;                                                      \
  else                                                                                    \
  {                                                                                       \
    float i = (d - value0) / (value1 - value0);                                           \
    r = (i <= 0.0) ? radius0 : (i >= 1.0) ? radius1 : radius0 + i*(radius1 - radius0);    \
  }                                                                                       \
}

namespace ospray {

  DataDrivenPathLines::DataDrivenPathLines()
  {
    this->ispcEquivalent = ispc::DataDrivenPathLines_create(this);
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

    bool useCurve = getParam1i("smooth", 0);


    with_data = getParam1i("with_data", 1);
    
    radius0 = getParam1f("radius0", globalRadius);
    radius1 = getParam1f("radius1", 0.0);
    value0 = getParam1f("value0", 0.0);
    value1 = getParam1f("value1", 0.0);

    vertexData = getParamData("vertex",nullptr);
    if (!vertexData)
      throw std::runtime_error("DataDrivenPathLines must have 'vertex' array");

    if (vertexData->type != OSP_FLOAT4 && vertexData->type != OSP_FLOAT3A)
      throw std::runtime_error("ddpathlines 'vertex' must be type OSP_FLOAT4 or OSP_FLOAT3A");

    vertex = (vec3fa*)vertexData->data;
    numVertices = vertexData->numItems;

    float *data_base = NULL;

    if (vertexData->type == OSP_FLOAT4)
    {
      useCurve = true;

      // then the w slot contains either the radius or a data value

      if (with_data)
        data_base = ((float *)vertexData->data) + 3;
      else
        radius.reset((const float*)vertex + 3, 4);
    }

    indexData  = getParamData("index",nullptr);
    if (!indexData)
      throw std::runtime_error("ddpathlines must have 'index' array");

    if (indexData->type != OSP_INT)
      throw std::runtime_error("ddpathlines 'index' array must be type OSP_INT");

    index = (uint32*)indexData->data;
    numSegments = indexData->numItems;

    colorData = getParamData("vertex.color",getParamData("color"));
    if (colorData && colorData->type != OSP_FLOAT4)
      throw std::runtime_error("'vertex.color' must have data type OSP_FLOAT4");
    const ispc::vec4f* color = colorData ? (ispc::vec4f*)colorData->data : nullptr;

    radiusData = getParamData("vertex.radius");
    if (radiusData && radiusData->type == OSP_FLOAT) {
      radius.reset((const float*)radiusData->data);
      useCurve = true;
    }

    // postStatusMsg(2) << "#osp: creating ddpathlines geometry, "
    std::cerr << "#osp: creating ddpathlines geometry, "
                     << "#verts=" << numVertices << ", "
                     << "#segments=" << numSegments << ", "
                     << "as curve: " << useCurve;

    bounds = empty;

    // XXX curves may actually have a larger bounding box due to swinging

    // If the w slot contains data, we need to map it to get per-vertex radius

    if (with_data)
    {
      float *base = ((float*)vertexData->data) + 3;

      for (uint32_t i = 0; i < numVertices; i++, base += 4)
      {
        float radius, datavalue = *base;
        MAP_RADIUS (datavalue, radius);
        bounds.extend(vertex[i] - radius);
        bounds.extend(vertex[i] + radius);
      }
    }
    else
    {
      for (uint32_t i = 0; i < numSegments; i++)
      {
        const uint32 idx = index[i];
        bounds.extend(vertex[idx] - radius[idx]);
        bounds.extend(vertex[idx] + radius[idx]);
        bounds.extend(vertex[idx+1] - radius[idx+1]);
        bounds.extend(vertex[idx+1] + radius[idx+1]);
      }
    }

    if (useCurve) {
      vertexCurve.clear();
      indexCurve.resize(numSegments);
      bool middleSegment = false;
      vec3f tangent;
      for (uint32_t i = 0; i < numSegments; i++) {
        const uint32 idx = index[i];
        const vec3f start = vertex[idx];
        const vec3f end = vertex[idx+1];
        const float lengthSegment = length(start - end);

        float startRadius, endRadius;

        if (with_data)
        {
          float d0 = data_base[4*idx], d1 = data_base[4*(idx+1)];
          MAP_RADIUS (d0, startRadius);
          MAP_RADIUS (d1, endRadius);
        }
        else
        {
          startRadius = radius[idx];
          endRadius = radius[idx+1];
        }

        indexCurve[i] = vertexCurve.size();
        if (middleSegment) {
          vertexCurve.push_back(vec4f(start, startRadius));
          vertexCurve.push_back(vec4f(start + tangent,
                lerp(1.f/3, startRadius, endRadius)));
        } else { // start new curve
          const vec3f cap =  lerp(1.f+startRadius/lengthSegment, end, start);
          vertexCurve.push_back(vec4f(cap, 0.f));
          vertexCurve.push_back(vec4f(start, startRadius));
        }

        middleSegment = i+1 < numSegments && index[i+1] == idx+1;
        if (middleSegment) {
          const vec3f next = vertex[idx+2];
          const vec3f delta = (1.f/3)*(next - start);
          const float b = length(next - end);
          const float r = lengthSegment/(lengthSegment + b);
          vertexCurve.push_back(vec4f(end - r*delta,
                lerp(2.f/3, startRadius, endRadius)));
          tangent = (1.f-r)*delta;
        } else { // end curve
          vertexCurve.push_back(vec4f(end, endRadius));
          const vec3f cap = lerp(1.f+endRadius/lengthSegment, start, end);
          vertexCurve.push_back(vec4f(cap, 0.f));
        }
      }
      ispc::DataDrivenPathLines_setCurve(getIE(),model->getIE(),
          (const ispc::vec3fa*)vertexCurve.data(), vertexCurve.size(),
          indexCurve.data(), numSegments, index, color);
    } else
      ispc::DataDrivenPathLines_set(getIE(),model->getIE(), globalRadius,
          (const ispc::vec3fa*)vertex, numVertices, index, numSegments, color);
  }

  OSP_REGISTER_GEOMETRY(DataDrivenPathLines,ddpathlines);

} // ::ospray
