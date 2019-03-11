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

// Two types of functions map volume samples to a probability distribution: a 
// linear transformation that maps linearly with 0 at some data value and 1 at some
// other data value, with 0 everywhere else, and a gaussian transformation with a
// given mean and standard deviation.   Default is linear from the data value min
// to the data value max

#define TF_NONE     0
#define TF_LINEAR   1
#define TF_GAUSSIAN 2

class MHSampleClientServer : public MultiServerHandler
{
public:
  MHSampleClientServer(DynamicLibraryP dlp, int cfd, int dfd);
  static void init();
  std::string handle(std::string line);

  VolumeP volume;             // volume to be sampled
  ParticlesP particles;       // particles object to stash samples

  struct Args
  {
    Key   vk;             // Volume key
    Key   pk;             // Particles key
    float radius;         // Radius for particles
    int   tf_type;        // type of transfer function TF_LINEAR or TF_GAUSSIAN  
    float tf0;            // first parameter of transfer function
    float tf1;            // second parameter of transfer function
    float sigma;          // standard deviation of gaussian step 
    int   n_iterations;   // iteration limit - after the initial skipped iterations
    int   n_startup;      // initial iterations to ignore
    int   n_skip;         // only retain every n_skip'th successful sample
    int   n_miss;         // max number of successive misses allowed before termination
    float r, g, b, a;     // color for spheres
    float istep;          // steps along i, j, and k axes 
    float jstep;
    float kstep;
  } args;
};

}
