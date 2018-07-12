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


#include "dtypes.h"
#include "Application.h"

#include "Volume.h"
#include "MappedVis.h"
#include "Datasets.h"

#include "KeyedObject.h"

namespace gxy
{
KEYED_OBJECT_POINTER(VolumeVis)

class VolumeVis : public MappedVis
{
  KEYED_OBJECT_SUBCLASS(VolumeVis, MappedVis) 

public:
	~VolumeVis();
  
  virtual void initialize();

  void AddSlice(vec4f s)
  {
    slices.push_back(s);
  }

  void SetSlices(int n, vec4f *s)
  {
    slices.clear();
    for (int i = 0; i < n; i++)
      AddSlice(s[i]);
  }
  
  void GetSlices(int& n, vec4f*& s)
  {
    n = slices.size();
    s = slices.data();
  }

  void AddIsovalue(float iv)
  {
    isovalues.push_back(iv);
  }

  void SetIsovalues(int n, float *isos)
  {
    isovalues.clear();
    for (int i = 0; i < n; i++)
      AddIsovalue(isos[i]);
  }
  
  void GetIsovalues(int& n, float*& i)
  {
    n = isovalues.size();
    i = isovalues.data();
  }

  void SetVolumeRendering(bool yn) { volume_rendering = yn; }
  bool GetVolumeRendering() { return volume_rendering; }

  virtual bool local_commit(MPI_Comm);

protected:

	virtual void initialize_ispc();
	virtual void allocate_ispc();
	virtual void destroy_ispc();

  virtual void LoadFromJSON(Value&);
  virtual void SaveToJSON(Value&, Document&);

  virtual int serialSize();
  virtual unsigned char* serialize(unsigned char *ptr);
  virtual unsigned char* deserialize(unsigned char *ptr);

  bool volume_rendering;

  vector<vec4f> slices;
  vector<float> isovalues;
};
}

