// ========================================================================== //
//                                                                            //
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
#include <math.h>

#include <dtypes.h>

#include "rapidjson/document.h"

#include "Volume.h"
#include "Particles.h"
#include "Datasets.h"
#include "Filter.h"
#include "Sampler.h"
#include "Camera.h"

#include <time.h>

using namespace gxy;
using namespace std;

namespace gxy
{

class RaycastSampler : public Filter
{
  static int rcsfilter_index;
public:

  static void
  init() {}

  RaycastSampler(KeyedDataObjectP source)
  {
    std::stringstream ss;
    ss << "RaycastSampleFilter_" << rcsfilter_index++;
    name = ss.str();

    camera = Camera::NewP();
    sampler = Sampler::NewP();

    ParticlesP samples = Particles::NewP();
    samples->CopyPartitioning(source);
    sampler->SetSamples(samples);
    std::cerr << "SAMPLES KEY " << samples->getkey() << "\n";

    result = KeyedDataObject::Cast(samples);
  }

  ~RaycastSampler() { std::cerr << "RaycastSampler dtor\n"; }

  void
  Sample(rapidjson::Document& doc, KeyedDataObjectP);

private:
  SamplerP sampler = NULL;
  CameraP  camera = NULL;

};

}
