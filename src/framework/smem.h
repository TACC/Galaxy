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

/*! \file smem.h 
 * \brief convenience class for shared memory shared pointers (SharedP) in Galaxy
 */

#include <iostream>
#include <memory>

namespace gxy
{


//! convenience class for shared memory shared pointers (SharedP) in Galaxy
/*! \ingroup framework */
class smem
{
public:
  ~smem(); //!< default destructor

  //! returns a new shared pointer (SharedP) of size `n` bytes
  /*! \param n the size in bytes of the memory block pointed to by this SharedP */
	static std::shared_ptr<smem> New(int n) { return std::shared_ptr<smem>(new smem(n)); }

	//! get a pointer to the underlying data for this SharedP
	unsigned char *get() { return ptr; }
	//! get the size of the memory block pointed to by this SharedP
	int   get_size() { return sz;  }

private:
  smem(int n);
	unsigned char *ptr;
	int sz;
	int kk;
};

//! convenience type for shared pointers in Galaxy
/*! \ingroup framework */
typedef std::shared_ptr<smem> SharedP;

} // namespace gxy
