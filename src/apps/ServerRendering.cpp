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

#include "ServerRendering.h"
#include <pthread.h>

namespace gxy
{

static pthread_mutex_t serverrendering_lock = PTHREAD_MUTEX_INITIALIZER;

KEYED_OBJECT_CLASS_TYPE(ServerRendering)

void
ServerRendering::initialize()
{
  Rendering::initialize();
	max_frame = -1;
}

void
ServerRendering::AddLocalPixels(Pixel *p, int n, int f, int s)
{

	extern int debug_target;

  if (f >= max_frame)
	{
		max_frame = f;

		char* ptrs[] = {(char *)&n, (char *)&f, (char *)&s, (char *)p};
		int   szs[] = {sizeof(int), sizeof(int), sizeof(int), static_cast<int>(n*sizeof(Pixel)), 0};

		socket->SendV(ptrs, szs);
		Rendering::AddLocalPixels(p, n, f, s);
	}
}

} // namespace gxy
