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

/*! \file ISPCObject.h 
 * \brief base class for data objects that will be passed to OSPRay for processing within ISPC routines
 * \ingroup data
 */

#include <string>
#include <string.h>
#include <vector>
#include <memory>

namespace gxy
{


//! base class for data objects that will be passed to OSPRay for processing within ISPC routines
/*! Galaxy utilizes the Intel OSPRay and Embree ray tracing engines, 
 * both of which use the Intel ISPC parallel language for instruction-level parallelism.
 * This class serves as a base for Galaxy data objects to ease ISPC integration.
 * \ingroup data
 */
class ISPCObject 
{
public:
	//! default constructor
  ISPCObject() { ispc = NULL; }
  virtual ~ISPCObject() { destroy_ispc(); } //!< default destructor

  //! return a pointer to the ISPC environment
  void *GetISPC() { return ispc; }

protected:
  virtual void allocate_ispc();
  virtual void initialize_ispc();
  virtual void destroy_ispc();

  void *ispc;
};

} // namespace gxy
