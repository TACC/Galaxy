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

/*! 
 *  This is the initial class that abstracts the operation of galaxy from
 *  ray-based renderer to ray-based sampler.
 *
 *  At the moment the design is simply to take over some of the operations 
 *  of the renderer and create samples instead of pixels. A more thorough
 *  discussion will follow as the design is worked out.
 *  \file Sampler.h
 *  \brief Initial class for ray-based sampling
 *  \ingroup applications
 */

#include <vector>
#include <pthread.h>

#include "Renderer.h"

namespace gxy
{

class RayList;

OBJECT_POINTER_TYPES(Sampler)

class Sampler : public Renderer
{
  KEYED_OBJECT_SUBCLASS(Sampler, Renderer)
    
public:
  static void Initialize();
  // virtual void initialize(); //!< initialize this object
  virtual void HandleTerminatedRays(RayList *raylist);
  void SetSamples(ParticlesP p) {mSamples = p;}
  ParticlesP GetSamples()
  {
    return mSamples;
  }

  virtual int SerialSize();
  virtual unsigned char *Serialize(unsigned char *);
  virtual unsigned char *Deserialize(unsigned char *);

  virtual void Sample(RenderingSetP);

private:
  ParticlesP mSamples = NULL;
  pthread_mutex_t lock;

  class SampleMsg : public Work
  {
  public:
    SampleMsg(Sampler *, RenderingSetP);
    ~SampleMsg() {}

    WORK_CLASS(SampleMsg, true);

    bool Action(int s);

    private:
        int frame;
  };

};

} // namespace gxy
