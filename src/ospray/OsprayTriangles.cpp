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

#include "OsprayTriangles.h"

using namespace gxy;

OsprayTriangles::OsprayTriangles(TrianglesP t)
{
  int nv = t->GetNumberOfVertices();
  int nc = t->GetConnectivitySize();

  if (nv > 0 && nc > 0)
  {
    
    OSPData pdata = ospNewData(nv, OSP_FLOAT3, t->GetVertices(), OSP_DATA_SHARED_BUFFER);
    ospCommit(pdata);

    OSPData ndata = ospNewData(nv, OSP_FLOAT3, t->GetNormals(), OSP_DATA_SHARED_BUFFER);
    ospCommit(ndata);
   
    OSPData ddata = ospNewData(nv, OSP_FLOAT4, t->GetData(), OSP_DATA_SHARED_BUFFER);
    ospCommit(ddata);

    OSPData tdata = ospNewData(nc/3, OSP_INT3, t->GetConnectivity(), OSP_DATA_SHARED_BUFFER);
    ospCommit(tdata);
 
    OSPGeometry ospg = ospNewGeometry("ddtriangles");

    ospSetData(ospg, "vertex", pdata);
    ospSetData(ospg, "vertex.normal", ndata);
    ospSetData(ospg, "index", tdata);
    ospSetData(ospg, "data", ddata);

    ospCommit(ospg);

    theOSPRayObject = (OSPObject)ospg;
  }
  else
    theOSPRayObject = nullptr;
}
