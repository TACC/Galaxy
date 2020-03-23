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

#include <iostream>
#include <OsprayHandle.h>
#include <ospray/ospray.h>

namespace gxy
{

class OsprayHandle;
std::weak_ptr<OsprayHandle> theOSPRayHandle;

class OsprayHandle
{
public:
  static auto deleter(OsprayHandle *p) { delete p; }
  
  OsprayHandle()
  {
    ospInit(0, NULL);
  }

  ~OsprayHandle()
  {
    ospShutdown();
  }
};

OsprayHandleP GetOspray()
{
  OsprayHandleP f = theOSPRayHandle.lock();
  if (! f)
  {
    f = OsprayHandleP(new OsprayHandle, OsprayHandle::deleter);
    theOSPRayHandle = f;
  }
  return f;
}

}


