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

#include "vec.ih"

struct TransferFunction_ispc
{
    uniform int   length;
    uniform int   width;
    uniform float minV;
    uniform float maxV;
    uniform float denom;
    uniform float *data;
};

varying float TransferFunction_SampleO(uniform TransferFunction_ispc* self, varying float v)
{
    if (v < self->minV) v = self->minV;
    else if (v > self->maxV) v = self->maxV;

    varying float d = ((v - self->minV) * self->denom) * (self->length - 1);
    varying int   i = (int)d;

    d = d - i;
    return self->data[i];

    return self->data[i] + ((d == 0.0) ? 0 : d*(self->data[i+1] - self->data[i]));
}

varying vec3f TransferFunction_SampleRGB(uniform TransferFunction_ispc* self, varying float v)
{
    uniform vec3f *data = (vec3f *)self->data;

    if (v < self->minV) v = self->minV;
    else if (v > self->maxV) v = self->maxV;

    varying float d = (v - self->minV) * self->denom;
    varying int   i = (int)d;
    d = d - i;

    if (i < 0)
        return data[0];
    else if (i >= self->length)
        return data[self->length-1];
    else
        return data[i] + d * (data[i+1] - data[i]);
}

varying vec4f TransferFunction_SampleRGBO(uniform TransferFunction_ispc* self, varying float v)
{
    uniform vec4f *data = (vec4f *)self->data;

    if (v < self->minV) v = self->minV;
    else if (v > self->maxV) v = self->maxV;

    varying float d = (v - self->minV) * self->denom;
    varying int   i = (int)d;
    d = d - i;

    if (i < 0)
        return data[0];
    else if (i >= self->length)
        return data[self->length-1];
    else
        return data[i] + d * (data[i+1] - data[i]);
}

export void *uniform TransferFunction_Test(uniform TransferFunction_ispc *uniform self, const uniform int n, float *uniform values, float *uniform results)
{
    if (self->width == 1)
    {
        foreach (i = 0 ... n)
            results[i] = TransferFunction_SampleO(self, values[i]);
    }
    else if (self->width == 3)
    {
        vec3f *r = (vec3f *)results;
        foreach (i = 0 ... n)
            r[i] = TransferFunction_SampleRGB(self, values[i]);
    }
    else
    {
        vec4f *r = (vec4f *)results;
        foreach (i = 0 ... n)
            r[i] = TransferFunction_SampleRGBO(self, values[i]);
    }
}
        




