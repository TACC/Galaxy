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

#ifdef GXY_OBJECT_REF_LIST

//
// The following stuff can be used to find hanging objects.   It maintains lists of allocated KeyedObjects
// of each type.   You can call dol() to print out a row for each non-empty type and shows the number of
// references on each existing object of that type.   This data structure itself has a reference on each.
// 
// aol(KeyedObjectP&) adds a newly created object to the data structure
// rmol(int type) removes the data structure's reference to each object of type 
//
std::vector<KeyedObjectP> object_lists[32];
extern void rmol(int);
void dol()
{
  std::cerr << "=========== " << GetTheApplication()->GetRank() << " ===========\n";
  for (auto i = 0; i < 32; i++)
  {
    auto ol = object_lists[i];
    if (ol.size() > 0)
    {
      std::cerr << GetTheKeyedObjectFactory()->GetClassName(i) << " (" << i << ") :";
      for (auto j = 0; j < ol.size(); j++)
        std::cerr << " " << (ol[j].use_count() - 1);  // -1 to ignore this local reference
      std::cerr << "\n";
    }
  }
  std::cerr << "\n";
}
void aol(KeyedObjectP& p)
{
  std::vector<KeyedObjectP>::iterator it;
  for (it = object_lists[p->getclass()].begin(); it != object_lists[p->getclass()].end(); it++)
    if (*it == p)
      break;
  if (it == object_lists[p->getclass()].end())
    object_lists[p->getclass()].push_back(p);
}

void rmol(int i)
{
  object_lists[i].clear();
}

#endif

static int ko_count = 0;
static int get_ko_count() { return ko_count; }

WORK_CLASS_TYPE(KeyedObject::CommitMsg)
WORK_CLASS_TYPE(KeyedObjectFactory::NewMsg)
WORK_CLASS_TYPE(KeyedObjectFactory::DropMsg)

KeyedObjectFactory* GetTheKeyedObjectFactory()
{
  return GetTheApplication()->GetTheKeyedObjectFactory();
}

int
KeyedObjectFactory::register_class(KeyedObject *(*n)(Key), std::string s)
{
  for (auto i = 0; i < class_names.size(); i++)
    if (class_names[i] == s)
    {
      if (new_procs[i] != n)
        new_procs[i] = n;

      return i;
    }

  new_procs.push_back(n);
  class_names.push_back(s);
  return new_procs.size() - 1;
}

bool
KeyedObjectFactory::NewMsg::CollectiveAction(MPI_Comm comm, bool isRoot)
{
	if (!isRoot)
	{
		unsigned char *p = (unsigned char *)get();
		KeyedObjectClass c = *(KeyedObjectClass *)p;
		p += sizeof(KeyedObjectClass);
		Key k = *(Key *)p;
		KeyedObjectP kop = shared_ptr<KeyedObject>(GetTheKeyedObjectFactory()->New(c, k));
	}

	return false;
}

// Remove any objects references from the key map.
void
KeyedObjectFactory::Clear()
{
  // Delete remote dependents of local *primary* objects.

  for (auto i = smap.begin(); i != smap.end(); i++)
    if (*i)
      (*i)->Drop();
}

void
KeyedObjectFactory::Drop(Key k)
{
#ifdef GXY_LOGGING
	APP_LOG(<< "DROP " << k);
#endif
	DropMsg msg(k);
	msg.Broadcast(true, true);
}

bool 
KeyedObjectFactory::DropMsg::CollectiveAction(MPI_Comm comm, bool isRoot)
{
	unsigned char *p = (unsigned char *)get();
	Key k = *(Key *)p;
	GetTheKeyedObjectFactory()->erase(k);
	return false;
}

// Get an object by key... could be in either keymap
KeyedObjectP
KeyedObjectFactory::get(Key k)
{
	KeyedObjectP kop = k >= smap.size() ? nullptr : smap[k];
  if (kop == nullptr)
		return k >= wmap.size() ? nullptr : wmap[k].lock();
	else
    return kop;
}

void
KeyedObjectFactory::erase(Key k)
{
	if (k < smap.size()) smap[k] = nullptr;
}

void
KeyedObjectFactory::add_weak(KeyedObjectP& p)
{
  for (int i = wmap.size(); i <= p->getkey(); i++)
		wmap.push_back(KeyedObjectW());

	wmap[p->getkey()] = p;
}

void
KeyedObjectFactory::add_strong(KeyedObjectP& p)
{
  for (int i = smap.size(); i <= p->getkey(); i++)
		smap.push_back(NULL);

	smap[p->getkey()] = p;
}

void
KeyedObject::Drop()
{
	GetTheKeyedObjectFactory()->Drop(getkey());
}

KeyedObject::KeyedObject(KeyedObjectClass c, Key k) : keyedObjectClass(c), key(k)
{  
	ko_count++;
    // std::cerr << " + " << ko_count;
    error = 0;
	initialize();
}

KeyedObject::~KeyedObject()
{  
  // If this is the *primary* object, send out message to remove dependents
  if (!GetTheApplication()->IsQuitting() && primary)
    Drop();

	ko_count--;
    //std::cerr << " - " << ko_count;
}

bool
KeyedObject::local_commit(MPI_Comm c)
{
	return false;
}

KeyedObjectP
KeyedObject::Deserialize(unsigned char *p)
{
	Key k = *(Key *)p;
	p += sizeof(Key);

	KeyedObjectP kop = GetTheKeyedObjectFactory()->get(k);
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
	return sizeof(Key) + sizeof(int);
}

unsigned char *
KeyedObject::serialize(unsigned char *p)
{
	return p;
}

unsigned char *
KeyedObject::deserialize(unsigned char *p)
{
	return p;
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

  KeyedObjectP kop = GetTheKeyedObjectFactory()->get(k);

  if (! isRoot)
    kop->deserialize(p);

  return kop->local_commit(c);
}

void
KeyedObjectFactory::Dump()
{
  cerr << "STRONG keymap (" << smap.size() << ")\n";
	for (int i = 0; i < smap.size(); i++)
	{
		KeyedObjectP kop = smap[i];
		if (kop != NULL)
			cerr << "key " << i << " " << GetClassName(kop->getclass()) << " count " << kop.use_count() << endl;
	}

  cerr << "WEAK keymap (" << wmap.size() << ")\n";
	for (int i = 0; i < wmap.size(); i++)
	{
		KeyedObjectP kop = wmap[i].lock();
		if (kop != NULL)
			cerr << "key " << i << " " << GetClassName(kop->getclass()) << " count " << kop.use_count() << endl;
	}
}

KeyedObjectFactory::~KeyedObjectFactory() 
{
  // No idea why this is necessary.   Without it, when the wmap vector is destroyed a 
  // segfault occurs in the weak pointer part of the shared_ptr code.

  void *d = wmap.data();
  size_t s = wmap.size() * sizeof(wmap[0]);
  memset(d, 0, s);

	while (wmap.size() > 0)
		wmap.pop_back();

	while (smap.size() > 0)
		smap.pop_back();

  while (new_procs.size() > 0)
    new_procs.pop_back();

  while (class_names.size() > 0)
    class_names.pop_back();

	if (ko_count > 0)
  {
		cerr << ko_count << " shared objects remain!!!" << endl;
#ifdef GXY_OBJECT_REF_LIST
    dol();
#endif
  }
}

} // namespace gxy

