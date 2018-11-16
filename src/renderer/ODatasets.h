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

/*! \file ODatasets.h 
 * \brief container for local OSPRay data objects within the Galaxy Renderer
 * \ingroup data
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

OBJECT_POINTER_TYPES(ODatasets)

//! container for local OSPRay equivalents for datasets within the Galaxy OSPRay renderer
/*! \ingroup data 
 * \sa OSPRayObject
 */
class Datasets : public OSPRayObject
{
  GALAXY_OBJECT(ODatasets)

public:
  static ODatasetsP NewP(DatasetsP p) { return ODatasets::Cast(std::shared_ptr<ODatasets>(new ODatasets(p))); }

  OSPRayObject GetByKey(Key);
  void Add(Key k, OSPRayObject);

private:
  ODatseta(DatasetsP);

  std::map<Key, OSPRayObject> ospray_data;
};

}
