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

#include "MappedVis.ih"

export void *uniform MappedVis_allocate()
{
    MappedVis_ispc *uniform v = uniform new uniform MappedVis_ispc;
    return (void *)v;
}

export void MappedVis_initialize(void *uniform _self)
{
    MappedVis_ispc *uniform self = (uniform MappedVis_ispc *)_self;
    self->transferFunction = NULL;
}

export void MappedVis_set_transferFunction(void *uniform _self, void *uniform d)
{
    MappedVis_ispc *uniform self = (uniform MappedVis_ispc *)_self;
    self->transferFunction = d;
}                                                                                                                            


