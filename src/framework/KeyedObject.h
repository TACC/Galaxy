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

#pragma once 

/*! \file KeyedObject.h 
 * \brief base class for registered (i.e. tracked) Work objects within Galaxy
 */

#include <string>
#include <iostream>
#include <memory>
#include <vector>

#include "Work.h"

#include "rapidjson/document.h"

namespace gxy
{
class KeyedObject;
typedef std::shared_ptr<KeyedObject> KeyedObjectP;

typedef int KeyedObjectClass;
typedef long Key;


class KeyedObjectFactory;

//! return a pointer to the KeyedObjectFactory singleton
/*! \ingroup framework
 */
extern KeyedObjectFactory* GetTheKeyedObjectFactory();

//! base class for registered Work objects in Galaxy
/*! Galaxy maintains a global registry of objects in order to route Work to the appropriate process. 
 * In order to be tracked in the registry, the object should derive from KeyedObject and call the
 * KEYED_OBJECT_POINTER macro in its header file (outside of the class definition) and the 
 * KEYED_OBJECT or KEYED_OBJECT_SUBCLASS macro in its class definition.
 *
 * The creation and registration of KeyedObject classes are maintained by the KeyedObjectFactory class.
 *
 * \ingroup framework
 * \sa KeyedObjectFactory, Work
 */
class KeyedObject
{
public:
  KeyedObject(KeyedObjectClass c, Key k); //!< constructor
  virtual ~KeyedObject(); //!< destructor

  //! commit this object and send its registry to all processes
	static void Register()
	{
		CommitMsg::Register();
	};

  //! remove this object from the global registry
	virtual void Drop();

  //! return the class identifier int
  KeyedObjectClass getclass() { return keyedObjectClass; }
  //! return the class key (as long)
  Key getkey() { return key; }

  //! return the byte size of this object when serialized
  /*! \warning derived classes should not overload this method. Instead, implement an override of serialSize.
   */
  int SerialSize() { return serialSize(); }

  //! deserialize this object from a byte stream
  /*! \warning derived classes should implement an overload of deserialize. */
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
  virtual void Commit();
  //! commit this object to the local registry
  virtual bool local_commit(MPI_Comm);

	// only concrete subclasses have static LoadToJSON at abstract layer
  //! construct object from a Galaxy JSON specification
  virtual void LoadFromJSON(rapidjson::Value&) { std::cerr << "abstract KeyedObject LoadFromJSON" << std::endl; }
  //! save object to a Galaxy JSON specification
  virtual void SaveToJSON(rapidjson::Value&, rapidjson::Document&) { std::cerr << "abstract KeyedObject SaveToJSON" << std::endl; }

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

  KeyedObjectClass keyedObjectClass;
  Key key;
};

//! Factory class to create and maintain KeyedObject derived objects
/*! Galaxy maintains a global registry of objects in order to route Work to the appropriate process. 
 * In order to be tracked in the registry, the object should derive from KeyedObject and call the
 * KEYED_OBJECT_POINTER macro in its header file (outside of the class definition) and the 
 * KEYED_OBJECT or KEYED_OBJECT_SUBCLASS macro in its class definition.
 *
 * The creation and registration of KeyedObject classes are maintained by the KeyedObjectFactory class.
 *
 * \ingroup framework
 * \sa KeyedObject, Work, KEYED_OBJECT_POINTER, KEYED_OBJECT, KEYED_OBJECT_SUBCLASS
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
  int register_class(KeyedObject *(*n)(Key), std::string s)
  {
    new_procs.push_back(n);
    class_names.push_back(s);
    return new_procs.size() - 1;
  }

  //! commit all new and dropped keys to the local registry on each process
	static void Register()
	{
		NewMsg::Register();
		DropMsg::Register();
	}

  //! create a new instance of a KeyedObject-derived class
  /*! \param c the KeyedObject class id, which is created using the KEYED_OBJECT_TYPE macro
   * \sa KEYED_OBJECT_TYPE
   */
  KeyedObjectP New(KeyedObjectClass c)
  {
    Key k = keygen();
    KeyedObjectP kop = std::shared_ptr<KeyedObject>(new_procs[c](k));
    add(kop);

    NewMsg msg(c, k);
    msg.Broadcast(true, true);

    return kop;
  }

  //! create a new instance of a KeyedObject-derived class with a given Key
  /*! \param c the KeyedObject class id, which is created using the KEYED_OBJECT_TYPE macro
   * \param k the key id to use for this KeyedObject
   * \sa KEYED_OBJECT_TYPE
   */
  KeyedObjectP New(KeyedObjectClass c, Key k)
  {
    KeyedObjectP kop = std::shared_ptr<KeyedObject>(new_procs[c](k));
    add(kop);

    return kop;
  }

  //! returns the class name string for the KeyedObject-derived class
  std::string GetClassName(KeyedObjectClass c) 
  { 
    return class_names[c];
  }

  //! return a pointer to the registered KeyedObject corresponding to a given Key
  /*! \param k the key for the desired KeyedObject
   * \returns a pointer to the requested KeyedObject or `NULL` if not found
   */
  KeyedObjectP get(Key k);

  //! add a KeyedObject to the local registry
  /*! \param p a pointer to the KeyedObject to add
   */
  void add(KeyedObjectP p);

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
  std::vector<KeyedObjectP> kmap;

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

//! defines std::shared_ptr and std::weak_ptr for class
/*! \param typ the class name that has KeyedObject as an ancestor class
 * \ingroup framework
 */
#define KEYED_OBJECT_POINTER(typ)                                                         \
class typ;                                                                                \
typedef std::shared_ptr<typ> typ ## P;                                                    \
typedef std::weak_ptr<typ> typ ## W;


//! defines a ClassType registry tracker for class
/*! \param typ the class name that has KeyedObject as an ancestor class
 * \ingroup framework
 */
#define KEYED_OBJECT_TYPE(typ)                                                            \
int typ::ClassType;                                                        


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
#define KEYED_OBJECT_SUBCLASS(typ, parent)                                                \
                                                                                          \
protected:                                                                                \
  typ(Key k = -1) : parent(ClassType, k) {}                                               \
  typ(KeyedObjectClass c, Key k = -1) : parent(c, k) {}                                   \
                                                                                          \
private:                                                                                  \
  static KeyedObject *_New(Key k)                                                         \
  {                                                                                       \
    typ *t = new typ(k);                                                                  \
    t->initialize();                                                                      \
    return (KeyedObject *)t;                                                              \
  }                                                                                       \
                                                                                          \
public:                                                                                   \
  typedef parent super;                                                                   \
  KeyedObjectClass GetClass() { return keyedObjectClass; }                                \
  static bool IsA(KeyedObjectP a) { return dynamic_cast<typ *>(a.get()) != NULL; }        \
  static typ ## P Cast(KeyedObjectP kop) { return std::dynamic_pointer_cast<typ>(kop); }  \
  static typ ## P GetByKey(Key k) { return Cast(GetTheKeyedObjectFactory()->get(k)); }    \
  static void Register();                                                                 \
                                                                                          \
  std::string GetClassName()                                                              \
  {                                                                                       \
    return GetTheKeyedObjectFactory()->GetClassName(keyedObjectClass);                    \
  }                                                                                       \
                                                                                          \
  static typ ## P NewP()                                                                  \
  {                                                                                       \
    KeyedObjectP kop = GetTheKeyedObjectFactory()->New(ClassType);                        \
    return Cast(kop);                                                                     \
  }                                                                                       \
                                                                                          \
  static void RegisterClass()                                                             \
  {                                                                                       \
    typ::ClassType = GetTheKeyedObjectFactory()->register_class(typ::_New, std::string(#typ)); \
  }                                                                                       \
  static int  ClassType;

} // namespace gxy

