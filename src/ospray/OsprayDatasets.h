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

/*! \file OsprayDatasets.h 
 * \brief container for local OSPRay data objects within the Galaxy Renderer
 * \ingroup render
 */

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <memory>

// Note: the source object is a distributed keyed data object

#include "KeyedObject.h"
#include "Box.h"

namespace gxy
{

OBJECT_POINTER_TYPES(OsprayDatasets)

//! container for local OSPRay equivalents for datasets within the Galaxy OSPRay renderer
/*! \ingroup render 
 * \sa OsprayObject
 */
class OsprayDatasets : public OsprayObject
{
  GALAXY_OBJECT(OsprayDatasets)

public:
  static OsprayDatasetsP NewP(DatasetsP p) { return OsprayDatasets::Cast(std::shared_ptr<OsprayDatasets>(new OsprayDatasets(p))); }

  OsprayObject GetByKey(Key);
  void Add(Key k, OsprayObject);

private:
  OsprayDatasets(DatasetsP);

  std::map<Key, OsprayObject> ospray_data;
};

}
