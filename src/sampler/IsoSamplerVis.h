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

/*! \file IsoSamplerVis.h 
 * \brief a visualization element that uses a color map and opacity map to define its rendering behavior
 * \ingroup render
 */

#include <string>
#include <string.h>
#include <vector>
#include <memory>

#include "dtypes.h"
#include "SamplerVis.h"
#include "Datasets.h"
#include "KeyedObject.h"

#include "rapidjson/document.h"

namespace gxy
{

OBJECT_POINTER_TYPES(IsoSamplerVis)

//! a visualization element that identifies isosurface crossings as samples
/*! \ingroup render
 * \sa KeyedObject, KeyedDataObject, Vis, SamplerVis
 */
class IsoSamplerVis : public SamplerVis
{
  KEYED_OBJECT_SUBCLASS(IsoSamplerVis, SamplerVis)

public:

  virtual ~IsoSamplerVis(); //!< default destructor

  virtual void initialize(); //!< initialize this IsoSamplerVis object

  //! commit this object to the global registry across all processes
  virtual bool Commit(DatasetsP);

  //! set the tolerance for this IsoSamplerVis
  void SetTolerance(float t);

  //! get the tolerance for this IsoSamplerVis
  float GetTolerance();

  //! construct a IsoSamplerVis from a Galaxy JSON specification
  virtual bool LoadFromJSON(rapidjson::Value&);

  //! Set the vis' ownership of the OSPRay object and set any per-vis parameters on it
  virtual void SetTheOsprayDataObject(OsprayObjectP o);

  //! commit this object to the local registry
  virtual bool local_commit(MPI_Comm);

 protected:
  virtual void allocate_ispc();
  virtual void initialize_ispc();

  float tolerance;

  virtual int serialSize();
  virtual unsigned char *serialize(unsigned char *);
  virtual unsigned char *deserialize(unsigned char *);

};

} // namespace gxy

