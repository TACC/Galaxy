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

/*! \file VolumeVis.h 
 * \brief a visualization element operating on a regular-grid volumetric dataset within Galaxy
 * \ingroup render
 */

#include "Application.h"
#include "Datasets.h"
#include "dtypes.h"
#include "KeyedObject.h"
#include "MappedVis.h"
#include "Volume.h"

namespace gxy
{

OBJECT_POINTER_TYPES(VolumeVis)

//! a visualization element operating on a regular-grid volumetric dataset within Galaxy
/*! \ingroup render 
 * \sa MappedVis, Vis, KeyedObject, IspcObject, OsprayObject
 */
class VolumeVis : public MappedVis
{
  KEYED_OBJECT_SUBCLASS(VolumeVis, MappedVis) 

public:
	~VolumeVis(); //!< default destructor
  
  //! initialize this VolumeVis object
  virtual void initialize();

  //! add a slice plane to this VolumeVis object
  void AddSlice(vec4f s)
  {
    slices.push_back(s);
  }

  //! set the slice plane(s) to use in this VolumeVis object
  void SetSlices(int n, vec4f *s)
  {
    slices.clear();
    for (int i = 0; i < n; i++)
      AddSlice(s[i]);
  }
  
  //! get the slice plane(s) used in this VolumeVis object
  void GetSlices(int& n, vec4f*& s)
  {
    n = slices.size();
    s = slices.data();
  }

  //! add an isovalue to extract from this VolumeVis object
  void AddIsovalue(float iv)
  {
    isovalues.push_back(iv);
  }

  //! set the isovalue(s) to extract from this VolumeVis object
  void SetIsovalues(int n, float *isos)
  {
    isovalues.clear();
    for (int i = 0; i < n; i++)
      AddIsovalue(isos[i]);
  }
  
  //! get the isovalue(s) to extract from this VolumeVis object
  void GetIsovalues(int& n, float*& i)
  {
    n = isovalues.size();
    i = isovalues.data();
  }

  //! set whether to use direct volume rendering to render this VolumeVis
  void SetVolumeRendering(bool yn) { volume_rendering = yn; }
  //! is direct volume rendering used to render this VolumeVis?
  bool GetVolumeRendering() { return volume_rendering; }

  virtual bool local_commit(MPI_Comm);

protected:

	virtual void initialize_ispc();
	virtual void allocate_ispc();
	virtual void destroy_ispc();

  virtual bool LoadFromJSON(rapidjson::Value&);
  virtual void SaveToJSON(rapidjson::Value&, rapidjson::Document&);

  virtual int serialSize();
  virtual unsigned char* serialize(unsigned char *ptr);
  virtual unsigned char* deserialize(unsigned char *ptr);

  bool volume_rendering;

  std::vector<vec4f> slices;
  std::vector<float> isovalues;
};

} // namespace gxy

