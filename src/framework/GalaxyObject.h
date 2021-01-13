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

/*! \file GalaxyObject.h 
 * \brief A few things common to all Galaxy objects, including the object factory and the key/object map
 * \ingroup framework
 */

namespace gxy 
{

class ObjectFactory;
extern ObjectFactory *GetTheObjectFactory();

//! defines std::shared_ptr and std::weak_ptr for all Galaxy objects.  Used in class header file
/*! \param typ the class name 
 *  \ingroup framework
 */

#define OBJECT_POINTER_TYPES(typ)       \
class typ;                              \
typedef std::shared_ptr<typ> typ ## Ptr;

OBJECT_POINTER_TYPES(GalaxyObject)

typedef int  GalaxyObjectClass;

#define OBJECT_CLASS_TYPE(typ)          \
GalaxyObjectClass typ::ClassType;

//! defines the ClassType for all Galaxy objects.   Used in class implementation file
/*! \param typ the class name 
 *  \ingroup framework
 */

#define GALAXY_OBJECT_SUBCLASS(typ, parent)                                                       \
public:                                                                                           \
  typedef parent super;                                                                           \
  static typ ## Ptr Cast(GalaxyObjectPtr gop) { return std::dynamic_pointer_cast<typ>(gop); }     \
  static bool IsA(GalaxyObjectPtr a) { return dynamic_cast<typ *>(a.get()) != NULL; }             \
  static bool IsA(GalaxyObject* a) { return dynamic_cast<typ *>(a) != NULL; }                     \
  std::string GetClassName() { return std::string(#typ); }                                        \
  int         GetClassType() { return ClassType; }                                                \
  static int  ClassType;                                                                          \
                                                                                                  \
  static typ ## Ptr New()                                                                         \
  {                                                                                               \
    typ *t = new typ;                                                                             \
    t->initialize();                                                                              \
    return std::shared_ptr<typ>(t);                                                               \
  }                                                                                               \
                                                                                                  \
  static void RegisterClass()                                                                     \
  {                                                                                               \
    typ::ClassType = GetTheObjectFactory()->register_class(typ::_New, std::string(#typ));         \
  }                                                                                               \
                                                                                                  \
protected:                                                                                        \
  typ() : parent() {}                                                                             \
                                                                                                  \
private:                                                                                          \
  static GalaxyObject *_New()                                                                     \
  {                                                                                               \
    typ *t = new typ();                                                                           \
    t->initialize();                                                                              \
    return (GalaxyObject *)t;                                                                     \
  }                                                                                               \

enum ObserverEvent
{
  Deleted, Updated
};

class GalaxyObject
{
public:
  static GalaxyObjectPtr Cast(GalaxyObjectPtr gop) { return std::dynamic_pointer_cast<GalaxyObject>(gop); }
  static bool IsA(GalaxyObjectPtr a) { return true; }
  static bool IsA(GalaxyObject* a) { return true; }
  std::string GetClassName() { return std::string("GalaxyObject"); }
  static int  ClassType;

  GalaxyObject() {}
  ~GalaxyObject()
  {
    NotifyObservers(Deleted, NULL);
    for (auto o : observed)
      o->UnregisterObserver(this);
  }

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

  void Observe(GalaxyObjectPtr o) { Observe(o.get()); }
  void Observe(GalaxyObject *o)
  {
    for (auto i : observed)
      if (i == o)
      {
        std::cerr << "Observer is already observing this!\n";
        return;
      }

    observed.push_back(o);
    o->RegisterObserver(this);
  }

  void Unobserve(GalaxyObjectPtr o) { Unobserve(o.get()); }
  void Unobserve(GalaxyObject *o)
  {
    for (auto i = observed.begin(); i != observed.end(); i++)
      if (*i == o)
        observed.erase(i);
    for (auto i = o->observers.begin(); i != o->observers.end(); i++)
      if (*i == o)
        o->observers.erase(i);
  }

  virtual void Notify(GalaxyObject* o, ObserverEvent id, void *cargo)
  {
    switch(id)
    {
      case Deleted:
      {
        for (auto i = observed.begin(); i != observed.end(); i++)
          if (*i == o)
          {
            observed.erase(i);
            return;
          }
        std::cerr << "Observer received a Delete from an Observable it wasn't observing!\n";
        return;
      }

      default:
      {
        std::cerr << "Base-class Observer received a Notification that should have been handed by subclass\n";
        return;
      }
    }
  }
    
  void Notify(GalaxyObjectPtr o, ObserverEvent id, void *cargo) { Notify(o.get(), id, cargo); }

  void RegisterObserver(GalaxyObjectPtr o) { RegisterObserver(o.get()); }
  void RegisterObserver(GalaxyObject* o)
  {
    for (auto i = observers.begin(); i != observers.end(); i++)
      if (*i == o)
      {
        std::cerr << "Adding observer to observable that already has it!\n";
        return;
      }

    observers.push_back(o);
  }

  void UnregisterObserver(GalaxyObject *o)
  {
    for (auto i = observers.begin(); i != observers.end(); i++)
      if (*i == o)
      {
        observers.erase(i);
        return;
      }

    std::cerr << "Removing observer from observable that doesn't have it!\n";
    return;
  }

  void NotifyObservers(ObserverEvent id, void *cargo)
  {
    for (auto o : observers)
      o->Notify(this, id, cargo);
  }

  //! initialize this object (default has no action)
  virtual void initialize() { error = 0; }

  int get_error() { return error; }
  void set_error(int e) { error = e; }

protected:
  int error;

private:
  std::vector<GalaxyObject*> observed;
  std::vector<GalaxyObject*> observers;
};

#define GALAXY_OBJECT(typ)  GALAXY_OBJECT_SUBCLASS(typ, GalaxyObject)

} // namespace gxy

