
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

/*! \file KeyedObject.h 
 * \brief base class for registered (i.e. tracked) Work objects within Galaxy
 * \ingroup framework
 */

// FOR OBJECT DEBUGGING
// #define GXY_OBJECT_REF_LIST

/**********

Object ownership in a distributed system using keymaps

The "primary" process of an object is the process that calls

  KeyedObjectP New(KeyedObjectClass c)

to create a *primary* object, which assignes a new key and sends out
a message for all the *other* processes (dependent processes, with
respect to this object) to call

  KeyedObjectP New(KeyedObjectClass c, Key k)

to create a "dependent" objects.

We need to differentiate these cases because we want the management
of the original primary object in the primary process to control the
minimum lifetime of the dependent objects in the dependent processes -
that is, to have the dependent objects persist until all local 
dependent-process references disappear and the primary object itself
is actually deleted.

Consider the following case: on the primary node, we create a new
object through a KeyedObjectP, and add place it in a container
KeyedObject, and then allow the original KeyedObjectP to go out of
scope.  We want the dependent objects to persist as long as, and no
longer than, it is accessable through the container object.

From the point of view of the primary process, suppose the keymap
contains strong references.   Then there are *two* references on
the original object: one from the container, and one from keymap.
Deleting the container decrements the ref count on the original
object to 1, and it doesn't get deleted.  Yet the primary process has
no KeyedObjectP's on it, so has no means to access it - its hanging,
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
through a KeyedObjectP and sticks it in the keymap.   Since the
keymap references are *weak*, there's no actual ownership and when
the KeyedObjectP goes out of scope, the object is deleted and the
dependent object ceases to exist on the dependent process.

Alternatively, suppose the dependent processes' keymaps references
are *strong*.  Now when the CollectiveAction() returns, the keymap
will retain a reference on the object, even though there are no
actual KeyedObjectP's in the dependent process with ownership of
the object.  The object persists, and can be referenced by keys
passed across from the primary process.

So the appropriate behaviour is to use a *weak* keymap on the primary
process, and a *strong* keymap on the dependent processes.   When
a KeyedObjectP's destructor is called ON THE OWNING PROCESS, we
want the it to go away and to have all DEPENDENT objects go away
on the DEPENDENT processes.   Therefore, the destructor of a
KeyedObject should determine whether the object is primary or dependent,
and if its a primary, then it should send out a message telling all the
dependent processes to eliminate it from their *strong* keymap,
causing them to actually be deleted.

*********/

#include <string>
#include <iostream>
#include <vector>

#include "GalaxyObject.h"
#include "Work.h"

#include "rapidjson/document.h"

namespace gxy
{

#define KEYED_OBJECT_CLASS_TYPE(typ)                       \
  int typ::ClassType;                                      

OBJECT_POINTER_TYPES(KeyedObject)

typedef int  KeyedObjectClass;
typedef long Key;

class KeyedObjectFactory;

//! return a pointer to the KeyedObjectFactory singleton
/*! \ingroup framework
 */
extern KeyedObjectFactory* GetTheKeyedObjectFactory();

//! base class for registered Work objects in Galaxy
/*! Galaxy maintains a global registry of objects in order to route Work to the appropriate process. 
 * In order to be tracked in the registry, the object should derive from KeyedObject and call the
 * OBJECT_POINTER_TYPES macro in its header file (outside of the class definition) and the 
 * KEYED_OBJECT or KEYED_OBJECT_SUBCLASS macro in its class definition.
 *
 * The creation and registration of KeyedObject classes are maintained by the KeyedObjectFactory class.
 *
 * \ingroup framework
 * \sa KeyedObjectFactory, Work
 */
class KeyedObject : public GalaxyObject
{
  friend class KeyedObjectFactory;

  GALAXY_OBJECT(KeyedObject)

public:
  KeyedObject(KeyedObjectClass c, Key k); //!< constructor
  virtual ~KeyedObject(); //!< destructor

  //! commit this object and send its registry to all processes
	static void Register()
	{
		CommitMsg::Register();
	};

  //! return the class identifier int
  KeyedObjectClass getclass() { return keyedObjectClass; }
  //! return the class key (as long)
  Key getkey() { return key; }

  //! return the byte size of this object when serialized
  /*! \warning derived classes should not overload this method. Instead, implement an override of serialSize.
   */
  int SerialSize() { return serialSize(); }

  //! deserialize this object from a byte stream
  /*! \warning derived classes should implement an overload of deserialize.
   */
  static KeyedObjectP Deserialize(unsigned char *p);
  //! serialize this object to a byte stream for communication
  /*! \warning derived classes should not overload this method. Instead, implement an override of serialize.
   */  
  unsigned char *Serialize(unsigned char *p);

  //! initialize this object (default has no action)
  virtual void initialize() {}

  //! return the byte size of this object when serialized
  /*! \warning derived classes should overload this method to calculate object byte size
   */
  virtual int serialSize();
  //! serialize this object to a byte stream 
  /*! \warning derived classes should overload this method to serialize the object
   */
  virtual unsigned char *serialize(unsigned char *);
  //! deserialize this object from a byte stream 
  /*! \warning derived classes should overload this method to deserialize the object
   */
  virtual unsigned char *deserialize(unsigned char *);

  //! commit this object to the global registry across all processes
  virtual bool Commit();
  //! commit this object to the local registry
  virtual bool local_commit(MPI_Comm);

	// only concrete subclasses have static LoadToJSON at abstract layer
  //! construct object from a Galaxy JSON specification
  virtual bool LoadFromJSON(rapidjson::Value&) { std::cerr << "abstract KeyedObject LoadFromJSON" << std::endl; return false; }

  //! a helper class for global messages to deserialize and commit the KeyedObject to each process local registry
  class CommitMsg : public Work
  {
  public:
    CommitMsg(KeyedObject* kop);

    // defined in Work.h
    WORK_CLASS(CommitMsg, false);

  public:
    bool CollectiveAction(MPI_Comm c, bool isRoot);
  };

protected:
  KeyedObjectClass keyedObjectClass;
  Key key;
  bool primary;

private:
  //! remove this object from the global registry
	virtual void Drop();
};

#ifdef GXY_OBJECT_REF_LIST

// see doc in KeyedObject.cpp to understand what these are for

extern std::vector<KeyedObjectP> object_lists[32];
extern void aol(KeyedObjectP& p);

#else

#define aol(j)

#endif

//! Factory class to create and maintain KeyedObject derived objects
/*! Galaxy maintains a global registry of objects in order to route Work to the appropriate process. 
 * In order to be tracked in the registry, the object should derive from KeyedObject and call the
 * OBJECT_POINTER_TYPES macro in its header file (outside of the class definition) and the 
 * KEYED_OBJECT or KEYED_OBJECT_SUBCLASS macro in its class definition.
 *
 * The creation and registration of KeyedObject classes are maintained by the KeyedObjectFactory class.
 *
 * \ingroup framework
 * \sa KeyedObject, Work, OBJECT_POINTER_TYPES, KEYED_OBJECT, KEYED_OBJECT_SUBCLASS
 */

class KeyedObjectFactory
{
public:
	KeyedObjectFactory() { next_key = 0; } //!< constructor
	~KeyedObjectFactory(); //!< destructor

  //! add the class to the KeyedObject registry
  /*! \param n pointer to a KeyedObject-derived object instance
   * \param s class name of the passed object
   */
  int register_class(KeyedObject *(*n)(Key), std::string s);

  KeyedObjectP 
  NewP(std::string classname)
  {
    for (auto i = 0; i < class_names.size(); i++)
      if (class_names[i] == classname)
      {
        Key k = keygen();
        KeyedObjectP kop = std::shared_ptr<KeyedObject>(new_procs[i](k));
        kop->primary = true;
        aol(kop);
        add_weak(kop);
        NewMsg msg(i, k);
        msg.Broadcast(true, true);
        return kop;
      }
    return NULL;
  }

  //! commit all new and dropped keys to the local registry on each process
	static void Register()
	{
		NewMsg::Register();
		DropMsg::Register();
	}

  //! create a new instance of a KeyedObject-derived class 
  /*! \param c the KeyedObject class id, which is created using the OBJECT_CLASS_TYPE macro
   * \sa OBJECT_CLASS_TYPE
   */
  KeyedObjectP NewP(KeyedObjectClass c)
  {
    Key k = keygen();
    KeyedObjectP kop = std::shared_ptr<KeyedObject>(new_procs[c](k));
    kop->primary = true;
    aol(kop);
    add_weak(kop);

    NewMsg msg(c, k);
    msg.Broadcast(true, true);

    return kop;
  }

  //! create a new instance of a KeyedObject-derived class with a given Key
  /*! \param c the KeyedObject class id, which is created using the OBJECT_CLASS_TYPE macro
   * \param k the key id to use for this KeyedObject
   * \sa OBJECT_CLASS_TYPE
   */
  KeyedObjectP New(KeyedObjectClass c, Key k)
  {
    KeyedObjectP kop = std::shared_ptr<KeyedObject>(new_procs[c](k));
    kop->primary = false;
    aol(kop);
    add_strong(kop);

    return kop;
  }

  //! returns the class name string for the KeyedObject-derived class
  std::string GetClassName(KeyedObjectClass c) 
  { 
    return class_names[c];
  }

  //! drop all objects held by the keymap
  void Clear();

  //! return a pointer to the registered KeyedObject corresponding to a given Key
  /*! \param k the key for the desired KeyedObject
   * \returns a pointer to the requested KeyedObject or `NULL` if not found
   */
  KeyedObjectP get(Key k);

  //! add a KeyedObject to the appropriate key map
  /*! \param p a pointer to the KeyedObject to add
   */
  void add_weak(KeyedObjectP& p);
  void add_strong(KeyedObjectP& p);

  //! print the local KeyedObject registry to std::cerr
	void Dump();

  //! drop a KeyedObject from the global registry
  /*! This creates a DropMsg Work object and broadcasts to all processes to drop 
   * the KeyedObject represented by the given Key.
   * To remove a KeyedObject only from the local registry, use \ref erase.
   * \param k the Key corresponding to the desired KeyedObject to drop
   * \sa erase
   */
	void Drop(Key k);

  //! erase a KeyedObject from the local registry
  /*! This removes a KeyedObject *only* from the local registry. To remove the 
   * KeyedObject from the global registry, use Drop.
   * \param k the Key corresponding to the desired KeyedObject to erase
   * \sa Drop
   */
  void erase(Key k);

  //! generate a registration key for a KeyedObject
  Key keygen()
  {
    return (Key)next_key++;
  }

private:

  std::vector<KeyedObject*(*)(Key)> new_procs;
  std::vector<std::string> class_names;

  std::vector<KeyedObjectW> wmap; // map of weak references
  std::vector<KeyedObjectP> smap; // map of strong references

	int next_key;

public:

  //! helper class to broadcast new KeyedObject registrants to all processes
  class NewMsg : public Work
  {
  public:

    NewMsg(KeyedObjectClass c, Key k) : NewMsg(sizeof(KeyedObjectClass) + sizeof(Key))
    {
      unsigned char *p = (unsigned char *)get();
      *(KeyedObjectClass *)p = c;
      p += sizeof(KeyedObjectClass);
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

//! provides class members for a class that derives directly from KeyedObject
/*! \param typ the class name that derives directly from KeyedObject
 * \ingroup framework
 */
#define KEYED_OBJECT(typ)  KEYED_OBJECT_SUBCLASS(typ, KeyedObject)

//! provides class members for a class that has KeyedObject as an ancestor class
/*! \param typ the class name that has KeyedObject as an ancestor class
 * \param parent the parent class for the specified new class (can be KeyedObject or derivative)
 * \ingroup framework
 */

#define KEYED_OBJECT_SUBCLASS(typ, parent)                                                      \
                                                                                                \
public:                                                                                         \
  GALAXY_OBJECT_SUBCLASS(typ, parent)                                                           \
                                                                                                \
protected:                                                                                      \
  typ(Key k = -1) : parent(ClassType, k) {}                                                     \
  typ(KeyedObjectClass c, Key k = -1) : parent(c, k) {}                                         \
                                                                                                \
private:                                                                                        \
  static KeyedObject *_New(Key k)                                                               \
  {                                                                                             \
    typ *t = new typ(k);                                                                        \
    t->initialize();                                                                            \
    return (KeyedObject *)t;                                                                    \
  }                                                                                             \
                                                                                                \
public:                                                                                         \
  static typ ## P GetByKey(Key k) { return Cast(GetTheKeyedObjectFactory()->get(k)); }          \
  static void Register();                                                                       \
                                                                                                \
  static typ ## P NewP()                                                                        \
  {                                                                                             \
    KeyedObjectP kop = GetTheKeyedObjectFactory()->NewP(ClassType);                             \
    aol(kop);                                                                                   \
    return Cast(kop);                                                                           \
  }                                                                                             \
                                                                                                \
  static void RegisterClass()                                                                   \
  {                                                                                             \
    typ::ClassType = GetTheKeyedObjectFactory()->register_class(typ::_New, std::string(#typ));  \
  }                                                                                             


} // namespace gxy

