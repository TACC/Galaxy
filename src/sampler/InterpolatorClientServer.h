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

#pragma once

#include <iostream>

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <cstdlib>
#include <math.h>

#include <dtypes.h>

#include "Application.h"
#include "Volume.h"
#include "Particles.h"
#include "MultiServerHandler.h"

namespace gxy
{

class InterpolatorClientServer : public MultiServerHandler
{
public:
  InterpolatorClientServer(SocketHandler *sh);
  static void init();
  bool handle(std::string line, std::string&);

  VolumeDPtr volume;             // volume to be sampled
  ParticlesDPtr src;             // particles to interpolate
  ParticlesDPtr dst;             // result

  struct Args
  {
    Key   vk;                         // Volume key
    Key   sk;                         // Source particles key
    Key   dk;                         // Destinationb particles key  
    float istride, jstride, kstride;  // strides along i, j, and k axes 
    float ox, oy, oz;                 // origin of local partition
    float dx, dy, dz;                 // steps in X, Y and Z
  } args;
};

}
