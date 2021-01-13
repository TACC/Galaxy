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

#include <string>
#include <vector>

#include "KeyedObjectMap.h"

namespace gxy
{

KeyedObjectMap theKeyedObjectMap;
KeyedObjectMap* GetTheKeyedObjectMap() { return &theKeyedObjectMap; }

WORK_CLASS_TYPE(KeyedObjectMap::NewMsg)
WORK_CLASS_TYPE(KeyedObjectMap::DropMsg)

KeyedObjectPtr
KeyedObjectMap::NewDistributed(GalaxyObjectClass c)
{
  GalaxyObjectPtr gop = GetTheObjectFactory()->New(c);
  if (! gop)
  {
    std::cerr << "KeyedObjectMap::NewDistributed - GalaxyObjectClass is bad\n";
    exit(1);
  }

  KeyedObjectPtr kop = KeyedObject::Cast(gop);
  if (! kop)
  {
    std::cerr << "KeyedObjectMap::NewDistributed - GalaxyObjectClass is not a KeyedObject\n";
    exit(1);
  }

  Key k = next_key++;
  kop->setkey(k);
  add_weak(kop);

  NewMsg msg(c, k);
  msg.Broadcast(true, true);

  return kop;
}


void
KeyedObjectMap::erase(Key k)
{
    if (k < smap.size()) smap[k] = nullptr;
}

// Get an object by key... could be in either keymap

KeyedObjectPtr
KeyedObjectMap::get(Key k)
{
  KeyedObjectPtr kop = k >= smap.size() ? nullptr : smap[k];
  if (kop == nullptr)
    return k >= wmap.size() ? nullptr : wmap[k].lock();
  else
    return kop;
}

void
KeyedObjectMap::add_weak(KeyedObjectPtr& p)
{
  for (int i = wmap.size(); i <= p->getkey(); i++)
        wmap.push_back(KeyedObjectWPtr());

    wmap[p->getkey()] = p;
}

void
KeyedObjectMap::add_strong(KeyedObjectPtr& p)
{
  for (int i = smap.size(); i <= p->getkey(); i++)
        smap.push_back(NULL);

    smap[p->getkey()] = p;
}

// Remove any objects references from the key map.
void
KeyedObjectMap::Clear()
{
  // Delete remote dependents of local *primary* objects.

  for (auto i = smap.begin(); i != smap.end(); i++)
    if (*i)
      (*i)->Drop();
}

void
KeyedObjectMap::Drop(Key k)
{     
#ifdef GXY_LOGGING
    APP_LOG(<< "DROP " << k);
#endif
    DropMsg msg(k);
    msg.Broadcast(true, true);
}

bool
KeyedObjectMap::NewMsg::CollectiveAction(MPI_Comm comm, bool isRoot)
{
  // This is called when a distributed object is created.  On the primary
  // process the object already exists and is in the process' map as a
  // weak pointer.   On non-primary processes, we need to create a dependent 
  // object and add it to the process' map as a strong reference.

  if (!isRoot)
  {
    // If this is not the primary process

    unsigned char *p = (unsigned char *)get();
    GalaxyObjectClass c = *(GalaxyObjectClass *)p;
    p += sizeof(GalaxyObjectClass);
    Key k = *(Key *)p;

    GalaxyObjectPtr gop = GetTheObjectFactory()->New(c);

    KeyedObjectPtr kop = KeyedObject::Cast(gop);
    if (! kop)
    {
      std::cerr << "KeyedObjectMap::NewMsg::CollectiveAction :: GalaxyObjectClass led to non-keyed object\n";
      exit(1);
    }

    kop->setkey(k);
    GetTheKeyedObjectMap()->add_strong(kop);
  }

  return false;
}

bool
KeyedObjectMap::DropMsg::CollectiveAction(MPI_Comm comm, bool isRoot)
{
  unsigned char *p = (unsigned char *)get();
  Key k = *(Key *)p;
  GetTheKeyedObjectMap()->erase(k);
  return false;
}

void
KeyedObjectMap::Dump()
{
  std::cerr << "STRONG keymap (" << smap.size() << ")\n";
	for (int i = 0; i < smap.size(); i++)
	{
		KeyedObjectPtr kop = smap[i];
		if (kop != NULL)
			std::cerr << "key " << i << " " << GetTheObjectFactory()->GetClassName(kop->getclass()) << " count " << kop.use_count() << std::endl;
	}

  std::cerr << "WEAK keymap (" << wmap.size() << ")\n";
	for (int i = 0; i < wmap.size(); i++)
	{
		KeyedObjectPtr kop = wmap[i].lock();
		if (kop != NULL)
			std::cerr << "key " << i << " " << GetTheObjectFactory()->GetClassName(kop->getclass()) << " count " << kop.use_count() << std::endl;
	}
}



};
