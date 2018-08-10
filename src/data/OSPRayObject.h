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

/*! \file OSPRayObject.h 
 * \brief base class for data objects that will be passed to OSPRay
 * \ingroup data
 */

#include <ospray/ospray.h>

#include "KeyedDataObject.h"
#include "OSPUtil.h"

namespace gxy
{
	
KEYED_OBJECT_POINTER(OSPRayObject)

//! base class for data objects that will be passed to OSPRay 
/*! Galaxy utilizes the Intel OSPRay ray tracing engine. 
 * This class serves as a base for Galaxy data objects to ease OSPRay integration.
 * \ingroup data
 * \sa KeyedObject, KeyedDataObject
 */
class OSPRayObject : public KeyedDataObject
{
  KEYED_OBJECT_SUBCLASS(OSPRayObject, KeyedDataObject)

public:
	virtual void initialize(); //!< initialize this object
	virtual ~OSPRayObject(); //!< default destructor

	//! get the OSPRay representation of this object
	OSPObject GetOSP() { return theOSPRayObject; }
	//! get the ISPC-based OSPRay representation of this object
	void      *GetOSP_IE() { return osp_util::GetIE((void *)theOSPRayObject); }

protected:
	OSPObject theOSPRayObject;
};

} // namespace gxy
