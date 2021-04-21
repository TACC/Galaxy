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

class TransferFunction
{
public: 
    TransferFunction(int w, int h);
    ~TransferFunction();

    void Interpolate(int n, float *data);

    void SetMin(float v);
    void SetMax(float v);
    void SetMinMax(float m, float M);
    int GetWidth();
    int GetLength();
    int GetMinV();
    int GetMaxV();
    float *GetData();

    void *GetIspc() { return _ispc; }

private:
    void *_ispc;
};

}
