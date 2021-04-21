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
#include "TransferFunction_ispc.h"

namespace gxy
{

TransferFunction::TransferFunction(int w, int l)
{
  _ispc = malloc(sizeof(::ispc::TransferFunction_ispc));
  ::ispc::TransferFunction_ispc *ispc = (::ispc::TransferFunction_ispc *)GetIspc();

  ispc->width  = w;
  ispc->length = l;
  ispc->minV   = 0;
  ispc->maxV   = 0;
  ispc->data   = new float[w * l];
}

TransferFunction::~TransferFunction()
{
  if (_ispc)
  {
    free(((::ispc::TransferFunction_ispc *)GetIspc())->data);
    free(_ispc);
  }
}

void 
TransferFunction::Interpolate(int n, float *data)
{
  ::ispc::TransferFunction_ispc *ispc = (::ispc::TransferFunction_ispc *)GetIspc();

  float *inpt = data;
  float *outpt = ispc->data;

#define INDX(i,j) (i*ispc->width + j)

  ispc->minV = inpt[INDX(0,0)], ispc->maxV = inpt[INDX(n, 0)];

  int i0 = 0, i1 = 1;
  for (int i = 0; i < ispc->length; i++)
  {
    float x = ispc->minV + (i / float(ispc->length))*(ispc->maxV - ispc->minV);

    while (inpt[INDX(i1,0)] <= x)
      i0++, i1++;

    float d = (x - inpt[INDX(i0,0)]) / (inpt[INDX(i1,0)] - inpt[INDX(i0,0)]);

    for (int j = 0; j < ispc->width; j++)
      outpt[INDX(i,j)] = inpt[INDX(i0,j+1)] + d * (inpt[INDX(i1,j+1)] - inpt[INDX(i0,j+1)]);
  }
}

void TransferFunction::SetMin(float v) { ((::ispc::TransferFunction_ispc *)GetIspc())->minV = v; }
void TransferFunction::SetMax(float v) { ((::ispc::TransferFunction_ispc *)GetIspc())->maxV = v; }
void TransferFunction::SetMinMax(float m, float M) { SetMin(m); SetMax(M); }
int TransferFunction::GetWidth() { return ((::ispc::TransferFunction_ispc *)GetIspc())->width; }
int TransferFunction::GetLength() { return ((::ispc::TransferFunction_ispc *)GetIspc())->length; }
int TransferFunction::GetMinV() { return ((::ispc::TransferFunction_ispc *)GetIspc())->minV; }
int TransferFunction::GetMaxV() { return ((::ispc::TransferFunction_ispc *)GetIspc())->maxV; }
float *TransferFunction::GetData() { return ((::ispc::TransferFunction_ispc *)GetIspc())->data; }



}
