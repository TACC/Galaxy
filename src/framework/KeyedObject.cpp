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


#include "Application.h"
#include "KeyedObject.h"

using namespace rapidjson;
using namespace std;

namespace gxy
{

WORK_CLASS_TYPE(KeyedObject::CommitMsg)

void
KeyedObject::Drop()
{
  GetTheKeyedObjectMap()->Drop(getkey());
}

KeyedObject::~KeyedObject()
{  
  // If this is the *primary* object, send out message to remove dependents
  if (!GetTheApplication()->IsQuitting() && primary)
    Drop();
}

bool
KeyedObject::local_commit(MPI_Comm c)
{
  return false;
}

KeyedObjectDPtr
KeyedObject::Deserialize(unsigned char *p)
{
  Key k = *(Key *)p;
  p += sizeof(Key);

  KeyedObjectDPtr kop = GetTheKeyedObjectMap()->get(k);
  p = kop->deserialize(p);

  if (*(int *)p != 12345)
  {
    cerr << "ERROR: Could not deserialize KeyedObject " << endl;
    exit(1);
  }

  return kop;
}

unsigned char *
KeyedObject::Serialize(unsigned char *p)
{
  *(Key *)p = getkey();
  p += sizeof(Key);
  p = serialize(p);
  *(int*)p = 12345;
  p += sizeof(int);
  return p;
}

int
KeyedObject::serialSize()
{
  //! a KeyedObject can be sent as a message.   It must be 
  //! preceded by the class type so a recipient object
  //! can be created and deserialized into.  The message is 
  //! terminated by an int containing 12345 as an error check

  return super::serialSize() + sizeof(Key) + sizeof(int);
}

unsigned char *
KeyedObject::serialize(unsigned char *p)
{
  return super::serialize(p);
}

unsigned char *
KeyedObject::deserialize(unsigned char *p)
{
  return super::deserialize(p);
}

bool
KeyedObject::Commit()
{
  CommitMsg msg(this);
  msg.Broadcast(true, true);
  NotifyObservers(ObserverEvent::Updated, (void *)this);
  return get_error() == 0;
}

KeyedObject::CommitMsg::CommitMsg(KeyedObject* kop) : KeyedObject::CommitMsg::CommitMsg(kop->SerialSize())
{
  unsigned char *p = (unsigned char *)get();
  kop->Serialize(p);
}

bool
KeyedObject::CommitMsg::CollectiveAction(MPI_Comm c, bool isRoot)
{
  unsigned char *p = (unsigned char *)get();
  Key k = *(Key *)p;
  p += sizeof(Key);

  KeyedObjectDPtr kop = GetTheKeyedObjectMap()->get(k);

  if (! isRoot)
    kop->deserialize(p);

  return kop->local_commit(c);
}


} // namespace gxy

