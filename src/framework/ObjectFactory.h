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

#include "GalaxyObject.h"
#include "ObjectFactory.h"

namespace gxy 
{

//! The ObjectFactory class.   Whan a GalaxyObject subclass is implemented,
//! it defines a static GalaxyObjectClass variable in its implementation
//! C++ file using the OBJECT_CLASS_TYPE macro from GalaxyObject.h.
//! When the class is registered with the ObjectFactory (by calling the
//! RegisterClass static method of the subclass) a unique value is
//! assigned to this variable and is associated with the class' new_proc()
//! function in the ObjectFactory.  Afterward, the ObjectFactory can
//! create a new instance of any object class using its GalaxyObjectClass
//! value.   This enables an instance of a class on one process to be
//! serialized and sent to another process.  On the far side, the
//! GalaxyObjectClass value (from the serialized message) is used to
//! create a new dependent object and then use its deserialize method
//! to decode the remainder of the message.

/*! \file ObjectFactory.h 
 * \brief The object factory and the key/object map
 * \ingroup framework
 */

class GalaxyObject;
class ObjectFactory;

//! return a pointer to the ObjectFactory singleton
/*! \ingroup framework
 */
extern ObjectFactory* GetTheObjectFactory();

//! Factory class to create and maintain KeyedObject derived objects
/*! Galaxy maintains a global registry of objects in order to route Work to the appropriate process. 
 * In order to be tracked in the registry, the object should derive from KeyedObject and call the
 * OBJECT_POINTER_TYPES macro in its header file (outside of the class definition) and the 
 * KEYED_OBJECT or KEYED_OBJECT_SUBCLASS macro in its class definition.
 *
 * The creation and registration of KeyedObject classes are maintained by the GalaxyObjectFactory class.
 *
 * \ingroup framework
 * \sa KeyedObject, Work, OBJECT_POINTER_TYPES, KEYED_OBJECT, KEYED_OBJECT_SUBCLASS
 */

class ObjectFactory
{
public:
	ObjectFactory() {} //!< constructor
	~ObjectFactory() {} //!< destructor

  //! add the class to the KeyedObject registry
  /*! \param n pointer to a KeyedObject-derived object instance
   * \param s class name of the passed object
   */
  int register_class(GalaxyObject *(*n)(), std::string s);

  //! create a new instance of a GalaxyObject-derived class by its class name
  /*! \param classname the GalaxyObject class name, which is created using the OBJECT_CLASS_TYPE macro
   * \sa OBJECT_CLASS_TYPE
   */

  GalaxyObjectPtr
  New(std::string classname)
  {
    for (auto i = 0; i < class_names.size(); i++)
      if (class_names[i] == classname)
        return New(i);

    return NULL;
  }

  //! create a new instance of a GalaxyObject-derived class by its GalaxyObjectClass value
  /*! \param c the GalaxyObject class id, which is created using the OBJECT_CLASS_TYPE macro
   * \sa OBJECT_CLASS_TYPE
   */

  GalaxyObjectPtr
  New(GalaxyObjectClass c);

  //! returns the class name string for the KeyedObject-derived class

  std::string GetClassName(GalaxyObjectClass c) 
  { 
    if (c > class_names.size())
      return "unknown";
    else
      return class_names[c];
  }

  //! returns the class name string for the KeyedObject-derived class

  GalaxyObjectClass GetGalaxyObjectClass(std::string classname)
  { 
    for (auto i = 0; i < class_names.size(); i++)
      if (class_names[i] == classname)
        return i;
    return -1;
  }

  //! Removes all primary objects from map
  void Clear();

private:

  std::vector<GalaxyObject*(*)()> new_procs;
  std::vector<std::string> class_names;
};

} // namespace gxy

