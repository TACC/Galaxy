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

#include "OsprayPointSurface.h"

using namespace gxy;

OsprayPointSurface::OsprayPointSurface(ParticlesP p)
{
  OSPGeometry ospg = ospNewGeometry("psurface");
  if (! ospg) 
  {
    std::cerr << "Could not create psurface geometry!\n";
    exit(1);
  }

  OSPData points = ospNewData(p->GetNumberOfVertices(), OSP_FLOAT3, p->GetVertices(), OSP_DATA_SHARED_BUFFER);
  ospCommit(points);
  ospSetData(ospg, "points", points);

  ospCommit(ospg);

  theOSPRayObject = (OSPObject)ospg;
}
