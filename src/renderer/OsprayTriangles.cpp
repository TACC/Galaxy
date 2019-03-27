// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
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

#include "OsprayTriangles.h"

using namespace gxy;

OsprayTriangles::OsprayTriangles(TrianglesP t)
{
  triangles = t;

  int n_triangles = triangles->GetNumberOfTriangles();
  int n_vertices = triangles->GetNumberOfVertices();
  if (n_triangles > 0)
  {
    float *cptr, *colors = new float[n_vertices * 4];
    cptr = colors;
    for (int i = 0; i < n_vertices; i++)
    {
      *cptr++ = 173.0 / 255.0;
      *cptr++ = 224.0 / 255.0;
      *cptr++ = 255.0 / 255.0;
      *cptr++ = 1.0;
    }
    
    OSPData pdata = ospNewData(n_vertices, OSP_FLOAT3, triangles->GetVertices(), OSP_DATA_SHARED_BUFFER);
    ospCommit(pdata);

    OSPData ndata = ospNewData(n_vertices, OSP_FLOAT3, triangles->GetNormals(), OSP_DATA_SHARED_BUFFER);
    ospCommit(ndata);
   
    OSPData tdata = ospNewData(n_triangles, OSP_INT3, triangles->GetTriangles(), OSP_DATA_SHARED_BUFFER);
    ospCommit(tdata);
 
    OSPData cdata = ospNewData(n_vertices, OSP_FLOAT4, colors);
    ospCommit(cdata);
    delete[] colors;

    OSPGeometry ospg = ospNewGeometry("triangles");

    ospSetData(ospg, "vertex", pdata);
    ospSetData(ospg, "vertex.normal", ndata);
    ospSetData(ospg, "index", tdata);
    ospSetData(ospg, "color", cdata);

    ospCommit(ospg);

    theOSPRayObject = (OSPObject)ospg;
  }
  else
    theOSPRayObject = nullptr;
}
