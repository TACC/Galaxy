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

#include "TrianglesVis.ih"

export void *uniform TrianglesVis_allocate()
{
	TrianglesVis_ispc *uniform v = uniform new uniform TrianglesVis_ispc;
	return (void *)v;
}

export void TrianglesVis_initialize(void *uniform _self)
{
	TrianglesVis_ispc *uniform self = (uniform TrianglesVis_ispc *)_self;
}

export void TrianglesVis_destroy(void *uniform _self)
{
}
