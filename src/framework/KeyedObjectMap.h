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

#pragma once

#include <string>
#include <vector>

/**********

Object ownership in a distributed system using keymaps

The "primary" process of an object is the process that calls

  KeyedObjectDPtr New(GalaxyObjectClass c)

to create a *primary* object, which assignes a new key and sends out
a message for all the *other* processes (dependent processes, with
respect to this object) to call

  KeyedObjectDPtr New(GalaxyObjectClass c, Key k)

to create a "dependent" objects.

We need to differentiate these cases because we want the management
of the original primary object in the primary process to control the
minimum lifetime of the dependent objects in the dependent processes -
that is, to have the dependent objects persist until all local 
dependent-process references disappear and the primary object itself
is actually deleted.

Consider the following case: on the primary node, we create a new
object through a KeyedObjectDPtr, and add place it in a container
KeyedObject, and then allow the original KeyedObjectDPtr to go out of
scope.  We want the dependent objects to persist as long as, and no
longer than, it is accessable through the container object.

From the point of view of the primary process, suppose the keymap
contains strong references.   Then there are *two* references on
the original object: one from the container, and one from keymap.
Deleting the container decrements the ref count on the original
object to 1, and it doesn't get deleted.  Yet the primary process has
no KeyedObjectDPtr's on it, so has no means to access it - its hanging,
accessible only through the keymap via a key that should not be
visible to the application code.

Alternatively, the keymap may contain *weak* references.  In this
case, the original object is *not* referenced by the keymap, and
the deletion of the container will cause the object to be deleted.
Thus *weak* pointers are appropriate on the primary process.

From the point of view of a dependent process, suppose the keymap
is contains *weak* references.  When the primary process sends out
a message for the dependent processes to create dependent objects,
the CollectiveAction() method of the message creates a new object
through a KeyedObjectDPtr and sticks it in the keymap.   Since the
keymap references are *weak*, there's no actual ownership and when
the KeyedObjectDPtr goes out of scope, the object is deleted and the
dependent object ceases to exist on the dependent process.

Alternatively, suppose the dependent processes' keymaps references
are *strong*.  Now when the CollectiveAction() returns, the keymap
will retain a reference on the object, even though there are no
actual KeyedObjectDPtr's in the dependent process with ownership of
the object.  The object persists, and can be referenced by keys
passed across from the primary process.

So the appropriate behaviour is to use a *weak* keymap on the primary
process, and a *strong* keymap on the dependent processes.   When
a KeyedObjectDPtr's destructor is called ON THE OWNING PROCESS, we
want the it to go away and to have all DEPENDENT objects go away
on the DEPENDENT processes.   Therefore, the destructor of a
KeyedObject should determine whether the object is primary or dependent,
and if its a primary, then it should send out a message telling all the
dependent processes to eliminate it from their *strong* keymap,
causing them to actually be deleted.

*********/

#include "KeyedObject.h"

namespace gxy
{

class KeyedObjectMap
{
public:
    KeyedObjectMap() { next_key = 1; };
    ~KeyedObjectMap() {};

    //! Create a new distributed object.   Called on the object's primary process,
    //! this call creates a new primary object, assigns it a key, and adds it to the 
    //! weak map.   It then uses the NewMsg to use all *other* processes to create 
    //! dependent objects, assign them the key, and stick them into the processes' 
    //! strong map.

    KeyedObjectPtr NewDistributed(GalaxyObjectClass);

    //! remove all local and remote aspects of all ojects that are primerally located on the local process
    void Clear();

    //! remove a specific primary and all its dependencies
    void Drop(Key);

    //! retrieve a KeyedObject by key; can be in either strong or weak list
    KeyedObjectPtr get(Key);

    //! erase strong map reference to an object by key
    void erase(Key);

    //! add a keyed object to the weak map
    void add_weak(KeyedObjectPtr&);

    //! add a keyed object to the strong map
    void add_strong(KeyedObjectPtr&);

    //! print the local KeyedObject registry to std::cerr
	  void Dump();

    //! register local object classes
    static void Register();

private:

  int next_key;

  std::vector<KeyedObjectWPtr> wmap; // map of weak references
  std::vector<KeyedObjectDPtr> smap; // map of strong references

  //! helper class to broadcast new KeyedObject registrants to all processes

  class NewMsg : public Work
  {
  public:

    NewMsg(GalaxyObjectClass c, Key k) : NewMsg(sizeof(GalaxyObjectClass) + sizeof(Key))
    {
      unsigned char *p = (unsigned char *)get();
      *(GalaxyObjectClass *)p = c;
      p += sizeof(GalaxyObjectClass);
      *(Key *)p = k;
    }

    WORK_CLASS(NewMsg, false);

  public:
    bool CollectiveAction(MPI_Comm comm, bool isRoot);
  };

  //! helper class to broadcast dropped KeyedObject instances to all processes

  class DropMsg : public Work
  {
  public:

    DropMsg(Key k) : DropMsg(sizeof(Key))
    {
      unsigned char *p = (unsigned char *)get();
      *(Key *)p = k;
    }

    WORK_CLASS(DropMsg, false);

  public:
    bool CollectiveAction(MPI_Comm comm, bool isRoot);
  };
};

}





