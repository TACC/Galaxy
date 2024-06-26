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

#include "GuiRendering.h"
#include <pthread.h>

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(GuiRendering)

void
GuiRendering::initialize()
{
  // std::cerr << "GuiRendering ctor " << std::hex << ((long)this) << "\n";
  Rendering::initialize();
  pthread_mutex_init(&lock, NULL);
	max_frame = -1;
  handler = NULL;
  owner = 0;
}

GuiRendering::~GuiRendering()
{
  // std::cerr << "GuiRendering dtor " << std::hex << ((long)this) << "\n";
  handler = NULL;
  pthread_mutex_destroy(&lock);
}

void
GuiRendering::AddLocalPixels(Pixel *p, int n, int f, int s)
{
  pthread_mutex_lock(&lock);

  if (f >= max_frame)
	{
		max_frame = f;

		char* ptrs[] = {(char *)id.c_str(), (char *)&n, (char *)&f, (char *)&s, (char *)p};
		int   szs[] = {((int)id.size()) + 1, sizeof(int), sizeof(int), sizeof(int), static_cast<int>(n*sizeof(Pixel)), 0};

    if (handler)
    {
      handler->getTheSocketHandler()->DSendV(ptrs, szs);
    }
    else
      std::cerr << "no handler\n";
	}

  pthread_mutex_unlock(&lock);
}

} // namespace gxy
