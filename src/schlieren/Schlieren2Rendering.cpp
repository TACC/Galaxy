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

#include "Schlieren2Rendering.h"

namespace gxy
{
KEYED_OBJECT_CLASS_TYPE(Schlieren2Rendering)

void
Schlieren2Rendering::Register()
{
  RegisterClass();
}

void
Schlieren2Rendering::AddLocalPixels(Pixel *p, int n, int f, int s)
{
std::cerr << "SR " << n << "\n";

#if defined(GXY_EVENT_TRACKING)
  GetTheEventTracker()->Add(new LocalPixelsEvent(n, this->getkey(), f));
#endif

  if (! framebuffer)
  {
    cerr << "ERROR: Rendering::Schlieren2AddLocalPixel called by non-owner" << endl;
    exit(1);
  }

  if (f >= frame)
  {
    if (f > frame)
      frame = f;
   
    while (n-- > 0)
    {
      float x = p->r, y = p->g;
      int ix = floor(x), iy = floor(y);
      float dx = x - ix, dy = y - iy;

      if (ix >= 0 && ix < width)
      {
        if (iy > 0) framebuffer[(iy*width + ix) << 2] += (1.0 - dx) * (1.0 - dy);
        if (iy+1 < height) framebuffer[((iy+1)*width + ix) << 2] += (1.0 - dx) * dy;
      }

      if ((ix+1) >= 0 && (ix+1) < width)
      {
        if (iy > 0) framebuffer[(iy * width + (ix+1)) << 2] += dx * (1.0 - dy);
        if (iy+1 < height) framebuffer[((iy+1)*width + (ix+1)) << 2] += dx * dy;
      }

      p++;
    }

  }
}

}
