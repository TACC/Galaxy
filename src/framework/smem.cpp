// ========================================================================== //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 //
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

#include "smem.h"

#include <iostream>

#include "Application.h"

using namespace std;

namespace gxy
{

static int dbg = -2;
static int smbrk = -1;
static int k = 0;

void
smem_catch(){}

smem::~smem()
{
	if (dbg == 1)
	{
		APP_LOG(<< "smem dtor " << kk << " " << sz << " at " << std::hex << ((long)ptr));
	}

	if (kk == smbrk)
		smem_catch();

	if (ptr) 
	{
		free(ptr);
	}
}

static int nalloc = 0;

smem::smem(size_t n)
{
	kk = k++;

	if (dbg == -2)
	{
		if (getenv("GXY_SMEMDBG"))
		{
			int d = atoi(getenv("GXY_SMEMDBG"));
			dbg = (d == -1 || d == GetTheApplication()->GetRank()) ? 1 : 0;
		}
		if (getenv("GXY_SMEMBRK"))
			smbrk = atoi(getenv("GXY_SMEMBRK"));
	}

	if (kk == smbrk)
		smem_catch();

	if (n > 0)
		ptr = (unsigned char *)malloc(n);
	else
		ptr = NULL;

	sz = n;

	if (dbg == 1)
	{
		 APP_LOG(<< "smem ctor " << kk << " " << sz << " at " << std::hex << ((long)ptr));
	}
}

} // namespace gxy
