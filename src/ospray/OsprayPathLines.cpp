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

#include "OsprayPathLines.h"

using namespace gxy;

OsprayPathLines::OsprayPathLines(PathLinesDPtr p)
{
  OSPGeometry ospg = ospNewGeometry("ddpathlines");
  if (! ospg) 
  {
    std::cerr << "Could not create ddpathlines geometry!\n";
    exit(1);
  }

  int nv = p->GetNumberOfVertices();
  if (nv > 0)
  {
    OSPData vdata = ospNewData(nv, OSP_FLOAT3, p->GetVertices(), OSP_DATA_SHARED_BUFFER);
    ospCommit(vdata);
    ospSetData(ospg, "vertices", vdata);

    float R = 0.1;
    ospSet1f(ospg, "radius", R);

    OSPData ddata = ospNewData(nv, OSP_FLOAT, p->GetData(), OSP_DATA_SHARED_BUFFER);
    ospCommit(ddata);
    ospSetData(ospg, "data", ddata);

    int *segments = p->GetConnectivity();
    OSPData sdata = ospNewData(p->GetConnectivitySize(), OSP_INT, segments, OSP_DATA_SHARED_BUFFER);
    ospCommit(sdata);
    ospSetData(ospg, "indices", sdata);

    float *colors = new float[nv*4];
    for (int i = 0; i < nv; i++)
    {
      colors[(i<<2) + 0] = 1.0;
      colors[(i<<2) + 1] = 0.6;
      colors[(i<<2) + 2] = 0.6;
      colors[(i<<2) + 3] = 1.0;
    }

    OSPData cdata = ospNewData(nv, OSP_FLOAT4, colors);
    ospCommit(cdata);
    ospSetData(ospg, "vertex.color", cdata);

    delete[] colors;
  }
  ospCommit(ospg);

  theOSPRayObject = (OSPObject)ospg;
}
