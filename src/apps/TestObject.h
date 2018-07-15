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

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <Application.h>
#include <KeyedObject.h>

KEYED_OBJECT_POINTER(TestObject)

class TestObject : public gxy::KeyedObject
{
	KEYED_OBJECT(TestObject)

private:
	class DoitMsg : public gxy::Work
  {
  public:
    DoitMsg(Key k, string foo) : DoitMsg(sizeof(Key) + foo.length() + 1)
    {
      *(Key *)contents->get() = k;
      memcpy((char *)contents->get() + sizeof(Key), foo.c_str(), foo.length()+1);
    }

    WORK_CLASS(DoitMsg, true);

  public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {
			Key k = *(Key *)contents->get();
			char *foo = ((char *)contents->get()) + sizeof(k);
			APP_PRINT(<< ((int)k) << " " << foo);
      return false;
    }
  };

public:

	virtual void initialize();
	virtual ~TestObject();

	void doit()
	{
		DoitMsg m(getkey(), string("I am a TestObject"));
		m.Broadcast(true);
	}
};
