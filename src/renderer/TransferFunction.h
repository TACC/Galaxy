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

#include <iostream>

namespace gxy
{

/*! \File TransferFunction.cpp
 * TransformFunction is an ISPC implementation of a 1D lookup into a table of 1, 3, or 4-tuples.   Linear interpolation, arbitrary length.  Capped.
 */

#include "TransferFunction_ispc.h"

template<int WIDTH, int LENGTH>
class TransferFunction
{
public: 
    TransferFunction()
    {
      ispc.length = LENGTH; 
      ispc.width  = WIDTH;
      ispc.minV   = 0;
      ispc.maxV   = 0;
      ispc.data   = new float[LENGTH * WIDTH]; 
    }

    ~TransferFunction()
    {
      delete[] ispc.data;
    }

    void Interpolate(int n, float *data);

    void SetMin(float v) { ispc.minV = v; }
    void SetMax(float v) { ispc.maxV = v; }
    void SetMinMax(float m, float M) { SetMin(m); SetMax(M); }


    int GetWidth() { return ispc.width; }
    int GetLength() { return ispc.length; }
    int GetMinV() { return ispc.minV; }
    int GetMaxV() { return ispc.maxV; }
    float *GetData() { return ispc.data; }

    ispc::TransferFunction_ispc *GetIspc() { return &ispc; }

private:
    ispc::TransferFunction_ispc ispc;
};

}
