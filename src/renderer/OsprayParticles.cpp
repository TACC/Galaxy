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

#include "OsprayParticles.h"

using namespace gxy;

OsprayParticles::OsprayParticles(ParticlesP p)
{
  particles = p;

  OSPGeometry ospg = ospNewGeometry("ddspheres");
  // OSPGeometry ospg = ospNewGeometry("spheres");
  if (! ospg) 
  {
    std::cerr << "Could not create ddspheres geometry!\n";
    exit(1);
  }

  int n_samples;
  Particle *samples;
  particles->GetSamples(samples, n_samples);

  OSPData data = ospNewData(n_samples * sizeof(Particle), OSP_UCHAR, samples, OSP_DATA_SHARED_BUFFER);
  ospCommit(data);

  ospSetData(ospg, "spheres", data);
  ospSet1i(ospg, "offset_datavalue", 12);

  float r, g, b, a;
  particles->GetDefaultColor(r, g, b, a);

  unsigned int color = ((unsigned char)(a * 255) << 24) | 
                       ((unsigned char)(b * 255) << 16) | 
                       ((unsigned char)(g * 255) <<  8) | 
                        (unsigned char)(r * 255);

  unsigned int *colors = new unsigned int[n_samples];
  for (int i = 0; i < n_samples; i++)
    colors[i] = color;

  OSPData clr = ospNewData(n_samples, OSP_UCHAR4, colors);
  delete[] colors;
  ospCommit(clr);
  ospSetObject(ospg, "color", clr);
  ospRelease(clr);

  ospCommit(ospg);

  theOSPRayObject = (OSPObject)ospg;
}
