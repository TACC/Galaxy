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

/*! \file OsprayVolume.h 
 * \brief translation class for Galaxy Volume to OSPRay Volume
 * \ingroup render
 */

#include "Application.h"
#include "Volume.h"
#include "OsprayObject.h"

namespace gxy
{

OBJECT_POINTER_TYPES(OsprayVolume)

//! translation class for Galaxy Volume to OSPRay Volume
/*! \ingroup render 
 * \sa OsprayObject
 */
class OsprayVolume : public OsprayObject
{
  GALAXY_OBJECT(OsprayVolume)

public:
  static OsprayVolumeDPtr NewDistributed(VolumeDPtr p) { return OsprayVolume::DCast(std::shared_ptr<OsprayVolume>(new OsprayVolume(p))); }
  ~OsprayVolume();

private:
  OsprayVolume(VolumeDPtr);
  std::shared_ptr<unsigned char> samples;
};

}

