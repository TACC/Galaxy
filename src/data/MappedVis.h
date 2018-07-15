// ========================================================================== //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 //
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

#include <string>
#include <string.h>
#include <vector>
#include <memory>

#include "ospray/ospray.h"

#include "dtypes.h"
#include "Vis.h"
#include "Datasets.h"
#include "KeyedObject.h"

#include "rapidjson/document.h"

namespace gxy
{

KEYED_OBJECT_POINTER(MappedVis)

class MappedVis : public Vis
{
  KEYED_OBJECT_SUBCLASS(MappedVis, Vis)

public:

  virtual ~MappedVis();

  virtual void initialize();

  virtual void Commit(DatasetsP);

  void SetColorMap(int, vec4f *);
  void SetOpacityMap(int, vec2f *);

  virtual void LoadFromJSON(rapidjson::Value&);
  virtual void SaveToJSON(rapidjson::Value&, rapidjson::Document&);

  virtual bool local_commit(MPI_Comm);

 protected:
  virtual void allocate_ispc();
  virtual void initialize_ispc();
 
  vector<vec4f> colormap;
  vector<vec2f> opacitymap;

  virtual int serialSize();
  virtual unsigned char *serialize(unsigned char *);
  virtual unsigned char *deserialize(unsigned char *);

  OSPTransferFunction transferFunction;
  
};

} // namespace gxy

