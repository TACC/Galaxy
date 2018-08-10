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

/*! \file MappedVis.h 
 * \brief a Vis that uses a color map and opacity map to define its rendering behavior
 * \ingroup data
 */

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

//! a Vis that uses a color map and opacity map to define its rendering behavior
/*! \ingroup data
 * \sa KeyedObject, KeyedDataObject, Vis
 */
class MappedVis : public Vis
{
  KEYED_OBJECT_SUBCLASS(MappedVis, Vis)

public:

  virtual ~MappedVis(); //!< default destructor

  virtual void initialize(); //!< initialize this MappedVis object

  //! commit this object to the global registry across all processes
  virtual void Commit(DatasetsP);

  //! set the colormap for this MappedVis as an array of XRGB values
  void SetColorMap(int, vec4f *);
  //! set the opacity map for this MappedVis as an array of XO values
  void SetOpacityMap(int, vec2f *);

  //! construct a MappedVis from a Galaxy JSON specification
  virtual void LoadFromJSON(rapidjson::Value&);
  //! save this MappedVis to a Galaxy JSON specification 
  virtual void SaveToJSON(rapidjson::Value&, rapidjson::Document&);

  //! commit this object to the local registry
  virtual bool local_commit(MPI_Comm);

 protected:
  virtual void allocate_ispc();
  virtual void initialize_ispc();
 
  std::vector<vec4f> colormap;
  std::vector<vec2f> opacitymap;

  virtual int serialSize();
  virtual unsigned char *serialize(unsigned char *);
  virtual unsigned char *deserialize(unsigned char *);

  OSPTransferFunction transferFunction;
  
};

} // namespace gxy

