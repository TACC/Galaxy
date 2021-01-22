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

template<int WIDTH, int LENGTH>
void 
TransferFunction<WIDTH, LENGTH>::Interpolate(int n, float *data)
{
  float (*inpt)[WIDTH+1] = (float(*)[WIDTH+1])data;
  float (*outpt)[WIDTH] = (float(*)[WIDTH])ispc.data;

  ispc.minV = inpt[0][0], ispc.maxV = inpt[n-1][0];

  int i0 = 0, i1 = 1;
  for (int i = 0; i < LENGTH; i++)
  {
    float x = ispc.minV + (i / float(LENGTH))*(ispc.maxV - ispc.minV);

    while (inpt[i1][0] <= x)
      i0++, i1++;

    float d = (x - inpt[i0].x) / (inpt[i1].x - inpt[i0].x);

    for (int j = 0; j < WIDTH; j++)
      outpt[i][j] = inpt[i0][j+1] + d * (inpt[i1][j+1] - inpt[i0][j+1]);
  }
}

}
