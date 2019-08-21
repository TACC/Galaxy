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

#include "Renderer.h"

namespace gxy
{

class RayList;

OBJECT_POINTER_TYPES(Schlieren)

class Schlieren : public Renderer
{
  KEYED_OBJECT_SUBCLASS(Schlieren, Renderer)
    
public:
  static void Initialize();

  virtual bool LoadStateFromValue(rapidjson::Value&);
  virtual void SaveStateToValue(rapidjson::Value&, rapidjson::Document&);

  virtual void HandleTerminatedRays(RayList *raylist);

  virtual int SerialSize();
  virtual unsigned char *Serialize(unsigned char *);
  virtual unsigned char *Deserialize(unsigned char *);

  virtual void Trace(RayList *);

  void NormalizeImages(RenderingSetP);

protected:
  float GetFar() { return far; }
  void  SetFar(float f) { far = f; }

private:
  float far;

  //! a Work unit to instruct Galaxy processes to begin rendering
  class NormalizeSchlierenImagesMsg : public Work
  {
  public:
    NormalizeSchlierenImagesMsg(RenderingSetP rs) : NormalizeSchlierenImagesMsg(sizeof(Key))
    {
      unsigned char *p = (unsigned char *)contents->get();
      *(Key *)p = rs->getkey();
    }

    ~NormalizeSchlierenImagesMsg() {}

    WORK_CLASS(NormalizeSchlierenImagesMsg, true);

    bool CollectiveAction(MPI_Comm coll_comm, bool isRoot);

  private:
    int frame;
  };

};

} // namespace gxy
