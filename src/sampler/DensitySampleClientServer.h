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

class DensitySampleClientServer : public MultiServerHandler
{
public:
  DensitySampleClientServer(SocketHandler *);
  static void init();
  bool handle(std::string line, std::string&);

  VolumeP volume;             // volume to be sampled
  ParticlesP particles;       // particles object to stash samples

  struct Args
  {
    Key   vk;                   // Volume key
    Key   pk;                   // Particles key
    int   nSamples;             // total number of samples
    int   ni,  nj,  nk;         // non-ghosted counts along each local axis
    int   gi,  gj,  gk;         // ghosted counts along each local axis
    int   goi, goj, gok;        // offsets of non-ghosted in ghosted
    int   istep, jstep, kstep;  // strides in ghosted space
    float ox, oy, oz;           // local grid origin
    float dx, dy, dz;           // grid step size
  } args;
};

}
