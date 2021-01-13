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

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Work.h"
#include "GalaxyObject.h"
#include "ObjectFactory.h"

#include "rapidjson/document.h"

/*! \file GalaxyObject.h 
 * \brief A few things common to all Galaxy objects, including the object factory and the key/object map
 * \ingroup framework
 */

namespace gxy 
{

class KeyedObjectMap;
extern KeyedObjectMap *GetTheKeyedObjectMap();


//! defines std::shared_ptr and std::weak_ptr for all Galaxy objects.  Used in class header file
/*! \param typ the class name 
 *  \ingroup framework
 */

#define KEYED_OBJECT_POINTER_TYPES(typ)         \
OBJECT_POINTER_TYPES(typ)                       \
typedef std::shared_ptr<typ> typ ## DPtr;       \
typedef std::weak_ptr<typ> typ ## WPtr;         \
void Delete(typ ## DPtr& p);

KEYED_OBJECT_POINTER_TYPES(KeyedObject)

//! defines the ClassType for all Galaxy objects.   Used in class implementation file
/*! \param typ the class name 
 *  \ingroup framework
 */

#define GALAXY_OBJECT(typ)  GALAXY_OBJECT_SUBCLASS(typ, GalaxyObject)

/**********

Object ownership in a distributed system using keymaps

The "primary" process of an object is the process that calls

  KeyedObjectDPtr kop = NewDistributed(GalaxyObjectClass c)

to create a *primary* object, which assigns a new key and sends out
a message for all the *other* processes (dependent processes, with
respect to this object) to call

  KeyedObjectDPtr kop = ObjectFactory->New(GalaxyObjectClass c)
  kop->setkey(k)
  KeyedObjectMap->add_strong(kop)

to create the "dependent" objects.  More discussion in KeyedObjectMap.h

*********/

#define KEYED_OBJECT_CLASS_TYPE(typ)                       \
  int typ::ClassType;                                      

OBJECT_POINTER_TYPES(KeyedObject)

typedef long Key;

//! base class for registered Work objects in Galaxy
/*! Galaxy maintains a global registry of objects in order to route Work to the appropriate process. 
 * In order to be tracked in the registry, the object should derive from KeyedObject and call the
 * OBJECT_POINTER_TYPES macro in its header file (outside of the class definition) and the 
 * KEYED_OBJECT or KEYED_OBJECT_SUBCLASS macro in its class definition.
 *
 * The creation and registration of KeyedObject classes are maintained by the ObjectFactory class.
 *
 * \ingroup framework
 * \sa ObjectFactory, Work
 */
class KeyedObject : public GalaxyObject
{
  // friend class ObjectFactory;

  GALAXY_OBJECT(KeyedObject)

public:
  KeyedObject(GalaxyObjectClass c, Key k); //!< constructor
  virtual ~KeyedObject(); //!< destructor

  //! commit this object and send its registry to all processes
	static void Register()
	{
		CommitMsg::Register();
	};

  //! return the class identifier int
  GalaxyObjectClass getclass() { return keyedObjectClass; }

  //! return the object key (as long)
  Key getkey() { return key; }

  //! set the object key (as long)
  void setkey(Key k) { key = k; }

  //! return the byte size of this object when serialized
  /*! \warning derived classes should not overload this method. Instead, implement an override of serialSize.
   */
  int SerialSize() { return serialSize(); }

  //! remove this object from the global registry
	virtual void Drop();

  //! deserialize this object from a byte stream
  /*! \warning derived classes should implement an overload of deserialize.
   */
  static KeyedObjectDPtr Deserialize(unsigned char *p);
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
  GalaxyObjectClass keyedObjectClass;
  Key key;
  bool primary;

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
  GALAXY_OBJECT_SUBCLASS(typ, parent)                                                           \
                                                                                                \
public:                                                                                         \
  static typ ## DPtr DCast(KeyedObjectDPtr kop) { return std::dynamic_pointer_cast<typ>(kop); } \
                                                                                                \
protected:                                                                                      \
  typ(Key k) : parent(ClassType, k) {}                                                          \
  typ(GalaxyObjectClass c, Key k = -1) : parent(c, k) {}                                        \
                                                                                                \
public:                                                                                         \
  static typ ## DPtr GetByKey(Key k) { return DCast(GetTheKeyedObjectMap()->get(k)); }          \
  static void Register();                                                                       \
                                                                                                \
  static typ ## DPtr NewDistributed()                                                           \
  {                                                                                             \
    KeyedObjectDPtr kop = GetTheKeyedObjectMap()->NewDistributed(ClassType);                    \
    return DCast(kop);                                                                          \
  }                                                                                             \

} // namespace gxy

#include "KeyedObjectMap.h"

