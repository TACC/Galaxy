// ========================================================================== //
//                                                                            //
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

#include <iostream>
#include <sstream>

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>

#include <dtypes.h>

#include "rapidjson/document.h"

#include "Volume.h"
#include "Geometry.h"
#include "Datasets.h"
#include "Filter.h"

#include <time.h>

using namespace gxy;
using namespace std;

#define RNDM ((float)rand() / RAND_MAX) 

namespace gxy
{

class InterpolatorMsg : public Work
{

public:
  InterpolatorMsg(VolumeDPtr v, GeometryDPtr g) : InterpolatorMsg(2*sizeof(Key))
  {
    Key *keys = (Key *)contents->get();
    keys[0] = v->getkey();
    keys[1] = g->getkey();
  }

  WORK_CLASS(InterpolatorMsg, true);

  bool CollectiveAction(MPI_Comm, bool);
};

class Interpolator : public Filter
{
  static int interpolator_index;

public:

  static void
  init()
  {
    InterpolatorMsg::Register();
  }

  Interpolator()
  {
    std::stringstream ss;
    ss << "InterpolatorFilter_" << interpolator_index++;
    name = ss.str();
  }

  void SetVolume(VolumeDPtr v) { volume = v; }
  void SetPointSet(GeometryDPtr p) { result = p->Copy(); }

  void Interpolate();

  ~Interpolator() {}

  VolumeDPtr volume;
};

}
