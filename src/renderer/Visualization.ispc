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

#include "Visualization.ih"

export void *uniform Visualization_allocate()
{
    Visualization_ispc *uniform v = uniform new uniform Visualization_ispc;
    return (void *)v;
}

export void Visualization_initialize(void *uniform _self)
{
    Visualization_ispc *uniform self = (uniform Visualization_ispc *)_self;
    
    self->nVolumeVis = 0;
    self->volumeVis = NULL;
    self->nMappedVis = 0;
    self->mappedVis = NULL;
}


export void Visualization_destroy(void *uniform _self)
{
    Visualization_ispc *uniform self = (uniform Visualization_ispc *)_self;

    delete[] self->volumeVis;
    delete[] self->mappedVis;
}

export void Visualization_commit(void *uniform _self, 
                void *uniform _model,
                int uniform nv, void *uniform _v, 
                int uniform nm, void *uniform _m,
                float *uniform gmin, float *uniform gmax,
                float *uniform lmin, float *uniform lmax)
{
    Visualization_ispc *uniform self = (uniform Visualization_ispc *)_self;

    self->model = (uniform Model *)_model;

    if (self->nVolumeVis != nv)
    {
        if (self->volumeVis) delete[] self->volumeVis;
        if (nv) self->volumeVis = uniform new uniform pVolumeVis_ispc[nv];
        else self->volumeVis = NULL;
        self->nVolumeVis = nv;
    }
 
    {
        uniform pVolumeVis_ispc *uniform v = ( uniform pVolumeVis_ispc *uniform)_v;
        for (uniform int i = 0; i < nv; i++)
            self->volumeVis[i] = v[i];
    }

    if (self->nMappedVis != nm)
    {
        if (self->mappedVis) delete[] self->mappedVis;
        if (nm) self->mappedVis = uniform new uniform pMappedVis_ispc[nm];
        else self->mappedVis = NULL;
        self->nMappedVis = nm;
    }

    if (nm)
    {
        uniform pMappedVis_ispc *uniform m = ( uniform pMappedVis_ispc *uniform)_m;
        for (uniform int i = 0; i < nm; i++)
            self->mappedVis[i] = m[i];
    }

    self->global_bb.lower = make_vec3f(gmin[0], gmin[1], gmin[2]);
    self->global_bb.upper = make_vec3f(gmax[0], gmax[1], gmax[2]);

    self->local_bb.lower = make_vec3f(lmin[0], lmin[1], lmin[2]);
    self->local_bb.upper = make_vec3f(lmax[0], lmax[1], lmax[2]);
}

