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

#include "Work.h"

#include <iostream>

#include "Application.h"
#include "MessageManager.h"

using namespace std;

namespace gxy
{

Work::Work()
{
}

Work::Work(int n)
{
	if (n > 0)
		contents = smem::New(n);
	else
		contents = nullptr;
}

Work::Work(SharedP sp)
{
	contents = sp;
}

Work::Work(const Work* o)
{
	contents = o->contents;
	className = o->className;
	type = o->type;
}

int
Work::RegisterSubclass(string name, Work *(*d)(SharedP))
{
	return GetTheApplication()->RegisterWork(name, d);
}

void
Work::Allocate(int n)
{
  contents = smem::New(n);
}

void 
Work::Send(int i)
{
	Application *app = GetTheApplication();
  MessageManager *mm = app->GetTheMessageManager();
	mm->SendWork(this, i);
}

void 
Work::Broadcast(bool c, bool b)
{
	Application *app = GetTheApplication();
  MessageManager *mm = app->GetTheMessageManager();
	mm->BroadcastWork(this, c, b);
}

Work::~Work()
{
}

} // namespace gxy
