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

OBJECT_POINTER_TYPES(Schlieren2)

class Schlieren2 : public Renderer
{
  KEYED_OBJECT_SUBCLASS(Schlieren2, Renderer)
    
public:
  static void Initialize();
  virtual void initialize(); //!< initializes the singleton Renderer


  virtual bool LoadStateFromValue(rapidjson::Value&);
  virtual void SaveStateToValue(rapidjson::Value&, rapidjson::Document&);

  virtual void HandleTerminatedRays(RayList *raylist);

  virtual int serialSize();
  virtual unsigned char *serialize(unsigned char *);
  virtual unsigned char *deserialize(unsigned char *);

  virtual void Trace(RayList *);

  void NormalizeImages(RenderingSetP);

  float GetFar() { return far; }
  void  SetFar(float f) { far = f; }

  float GetCutoffValue() { return cutoff_value; }
  void SetCutoffValue(float f) { cutoff_value = f; }

  int GetRaysPerPixel() { return raysPerPixel; }
  void SetRaysPerPixel(int f) { raysPerPixel = f; }

  float GetCutoffType() { return cutoff_type; }
  void SetCutoffType(int t) { cutoff_type = t; }
  
  virtual void local_render(RendererP, RenderingSetP);

private:
  float far;
  int raysPerPixel;
  float cutoff_value;
  int cutoff_type;

  //! a Work unit to instruct Galaxy processes to begin rendering
  class NormalizeSchlieren2ImagesMsg : public Work
  {
  public:
    NormalizeSchlieren2ImagesMsg(RenderingSetP rs) : NormalizeSchlieren2ImagesMsg(sizeof(Key))
    {
      unsigned char *p = (unsigned char *)contents->get();
      *(Key *)p = rs->getkey();
    }

    ~NormalizeSchlieren2ImagesMsg() {}

    WORK_CLASS(NormalizeSchlieren2ImagesMsg, true);

    bool CollectiveAction(MPI_Comm coll_comm, bool isRoot);

  private:
    int frame;
  };

};

} // namespace gxy
