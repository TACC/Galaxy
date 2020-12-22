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

#include <iostream>

#include "TransferFunction.h"

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(TransferFunction)

void
TransferFunction::Register()
{
    RegisterClass();
}

void
TransferFunction::initialize()
{
    super::initialize();
    ispc.data = NULL;
}

TransferFunction::~TransferFunction()
{
    if (ispc.data) delete[] ispc.data;
}

int 
TransferFunction::serialSize()
{
    return super::serialSize() +  sizeof(ispc) + ispc.length*ispc.width*sizeof(float);
}

unsigned char*
TransferFunction::serialize(unsigned char *ptr)
{
    ptr = super::serialize(ptr);

    memcpy((void *)ptr, (void *)&ispc, sizeof(ispc));
    ptr = ptr + sizeof(ispc);

    memcpy((void *)ptr, (void *)ispc.data, ispc.length*ispc.width*sizeof(float));
    ptr = ptr + ispc.length*ispc.width*sizeof(float);
 
    return ptr;
}

unsigned char*
TransferFunction::deserialize(unsigned char *ptr)
{
    ptr = super::deserialize(ptr);

    if (ispc.data) delete[] ispc.data;

    memcpy((void *)&ispc, (void *)ptr, sizeof(ispc));
    ptr = ptr + sizeof(ispc);

    ispc.data = new float[ispc.length * ispc.width];
    memcpy((void *)ispc.data, (void *)ptr, ispc.length*ispc.width*sizeof(float));
    ptr = ptr + ispc.length*ispc.width*sizeof(float);

    return ptr;
}

void
TransferFunction::Set(int l, int w, float m, float M, float *data)
{
    if (m > M) 
    {
        std::cerr << "TransferFunction: min > max!\n";
        ispc.minV = M;
        ispc.maxV = m;
    }
    else if (m == M)
    {
        std::cerr << "TransferFunction: min == max!\n";
        ispc.minV = m;
        ispc.maxV = m;
        ispc.denom = 1;
    }
    else
    {
        ispc.minV   = m;
        ispc.maxV   = M;
        ispc.denom  = 1.0 / (M - m);
    }

    ispc.length = l;
    ispc.width  = w;
    ispc.data = new float[l * w];
    memcpy((void *)ispc.data, data, l*w*sizeof(float));
}

}
