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

#include "Vis.ih"

export void *uniform Vis_allocate()
{
    Vis_ispc *uniform v = uniform new uniform Vis_ispc;
    return (void *)v;
}

export void *uniform Vis_initialize(void *uniform _self)
{
    Vis_ispc *uniform self = (uniform Vis_ispc *)_self;
    self->data = NULL;
}

export void *uniform Vis_destroy(void *uniform _self)
{
    delete _self;
}

export void Vis_set_data(void *uniform _self, void *uniform d)
{
    Vis_ispc *uniform self = (uniform Vis_ispc *)_self;
    self->data = d;
}

 