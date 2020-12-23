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

#include "EmbreePathLines.h"

#include <embree3/rtcore.h>
#include <embree3/rtcore_common.h>

#include "EmbreePathLines_ispc.h"

namespace gxy 
{

EmbreePathLines::EmbreePathLines() : EmbreeGeometry()
{
}

void
EmbreePathLines::CreateIspc()
{
    if (! ispc)
        ispc = malloc(sizeof(ispc::EmbreePathLines_ispc));

    EmbreeGeometry::CreateIspc();
    
    ispc::EmbreePathLines_ispc *iptr = (ispc::EmbreePathLines_ispc*)ispc;

    iptr->epsilon = 0.001;
    iptr->radius0 = 0;
    iptr->value0  = 0;
    iptr->radius1 = 0;
    iptr->value1  = 0;
}

void
EmbreePathLines::SetMap(float v0, float r0, float v1, float r1)
{
    ispc::EmbreePathLines_ispc *iptr = (ispc::EmbreePathLines_ispc*)GetIspc();
    iptr->radius0 = r0;
    iptr->value0  = v0;
    iptr->radius1 = r1;
    iptr->value1  = v1;
}

#define MAP_RADIUS(d, i, r)                                   \
{                                                             \
  if (!d) r = r0;                                             \
  else if (v0 == v1) r = d[i];                                \
  else                                                        \
  {                                                           \
    float R = (d[i] - v0) * vd;                               \
    r = (R <= 0.0) ? r0 : (R >= 1.0) ? r1 : r + R*(r1 - r0);  \
  }                                                           \
}

#define LOWER(b, v)   \
{                     \
    b.lower_x = v.x;  \
    b.lower_y = v.y;  \
    b.lower_z = v.z;  \
}

#define UPPER(b, v)   \
{                     \
    b.upper_x = v.x;  \
    b.upper_y = v.y;  \
    b.upper_z = v.z;  \
}

#define EXTEND(b, v)                                                                 \
{                                                                                    \
    if (b.lower_x > v.x) b.lower_x = v.x; else if (b.upper_x < v.x) b.upper_x = v.x; \
    if (b.lower_y > v.y) b.lower_y = v.y; else if (b.upper_y < v.y) b.upper_y = v.y; \
    if (b.lower_z > v.z) b.lower_z = v.z; else if (b.upper_z < v.z) b.upper_z = v.z; \
}

#define LERP(r, a, b) (r*a + (1-r)*b)

void 
EmbreePathLines::FinalizeIspc()
{
    PathLinesP p = PathLines::Cast(geometry);
    if (! p)
    {
        std::cerr << "EmbreePathLines::FinalizeIspc called with something other than PathLines\n";
        exit(1);
    }

    EmbreeGeometry::FinalizeIspc();

    ispc::EmbreePathLines_ispc *iptr = (ispc::EmbreePathLines_ispc*)GetIspc();

    // XXX curves may actually have a larger bounding box due to swinging

    float v0 = iptr->value0;
    float r0 = iptr->radius0;
    float v1 = iptr->value1;
    float r1 = iptr->radius1;
    float e  = iptr->epsilon;

    vec3f* vertices = (vec3f *)iptr->super.vertices;

    float vd = (v0 == v1) ? 1.0 : 1 / (v1 - v0);
    
    RTCBounds bounds;
    for (int i = 0; i < iptr->super.nv; i++)
    {
        float r;
        MAP_RADIUS(iptr->super.data, i, r);
        if (i == 0)
        {
            vec3f vv = vertices[i] - r;
            LOWER(bounds, vv);
            vv = vertices[i] + r;
            UPPER(bounds, vv);
        }
        else
        {
            vec3f vv = vertices[i] - r;
            EXTEND(bounds, vv);
            vv = vertices[i] + r;
            EXTEND(bounds, vv);
        }
    }

    vCurve.clear();
    iCurve.clear();

    bool middleSegment = false;
    vec3f tangent;
    int pline = 0;

    for (uint32_t i = 0; i < iptr->super.nc; i++)
    {
      int   idx   = iptr->super.connectivity[i];
      vec3f start = vertices[idx];
      vec3f end   = vertices[idx+1];

      vec3f seg = start - end;
      float lengthSegment = len(seg);

      float startRadius, endRadius;

      MAP_RADIUS (iptr->super.data, idx, startRadius);
      MAP_RADIUS (iptr->super.data, idx+1, endRadius);

      iCurve.push_back(vCurve.size());

      if (middleSegment)
      {
        vCurve.push_back(vec4f(start.x, start.y, start.z, startRadius));
        vec3f nxt = start + tangent;
        vCurve.push_back(vec4f(nxt.x, nxt.y, nxt.z, LERP(1.f/3, startRadius, endRadius)));
      }
      else
      { 
        // start new curve
        vCurve.push_back(vec4f(start.x, start.y, start.z, startRadius));
        vCurve.push_back(vec4f(start.x, start.y, start.z, startRadius));
      }

      middleSegment = i+1 < iptr->super.nc && iptr->super.connectivity[i+1] == idx+1;

      if (middleSegment)
      {
        vec3f next = vertices[idx+2];

        vec3f delta = (next - start) * (1.f / 3);
        vec3f rem = next - end;
        float b = len(rem);
        float r = lengthSegment/(lengthSegment + b);

        vec3f nxt = end - delta*r;
        vCurve.push_back(vec4f(nxt.x, nxt.y, nxt.z, LERP(2.f/3, startRadius, endRadius)));

        tangent = delta*(1.f - r);
      }
      else { // end curve
        vCurve.push_back(vec4f(end.x, end.y, end.z, endRadius));
        vCurve.push_back(vec4f(end.x, end.y, end.z, endRadius));
      }
    }

    iptr->nvc = vCurve.size();
    iptr->vertexCurve = (float *)vCurve.data();

    iptr->ni = iCurve.size();
    iptr->indexCurve = (int *)iCurve.data();

    device_geometry = rtcNewGeometry((RTCDevice)GetEmbree()->Device(), RTC_GEOMETRY_TYPE_USER);

    device_geometry = rtcNewGeometry((RTCDevice)GetEmbree()->Device(), RTC_GEOMETRY_TYPE_ROUND_BEZIER_CURVE);
    rtcSetSharedGeometryBuffer(device_geometry, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT4, iptr->vertexCurve, 0, sizeof(vec4f), iptr->nvc);
    rtcSetSharedGeometryBuffer(device_geometry, RTC_BUFFER_TYPE_INDEX,  0, RTC_FORMAT_UINT, iptr->indexCurve, 0, sizeof(int), iptr->ni);
    rtcCommitGeometry(device_geometry);
}

}
