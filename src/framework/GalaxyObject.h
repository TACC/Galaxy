// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
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
 * \brief A few things common to all Galaxy objects
 * \ingroup framework
 */

namespace gxy 
{

//! defines std::shared_ptr and std::weak_ptr for all Galaxy objects.  Used in class header file
/*! \param typ the class name 
 *  \ingroup framework
 */

#define OBJECT_POINTER_TYPES(typ)                                                         \
class typ;                                                                                \
typedef std::shared_ptr<typ> typ ## P;                                                    \
typedef std::weak_ptr<typ> typ ## W;                                                      \
void Delete(typ ## P& p);

OBJECT_POINTER_TYPES(GalaxyObject)

//! defines the ClassType for all Galaxy objects.   Used in class implementation file
/*! \param typ the class name 
 *  \ingroup framework
 */

#define OBJECT_CLASS_TYPE(typ)                                                              \
int typ::ClassType;                                                                         \
void Delete(typ ## P& p) { p = NULL; }

#define GALAXY_OBJECT_SUBCLASS(typ, parent)                                                 \
public:                                                                                     \
  typedef parent super;                                                                     \
  static typ ## P Cast(GalaxyObjectP kop) { return std::dynamic_pointer_cast<typ>(kop); }   \
  static bool IsA(GalaxyObjectP a) { return dynamic_cast<typ *>(a.get()) != NULL; }         \
  static bool IsA(GalaxyObject* a) { return dynamic_cast<typ *>(a) != NULL; }               \
  std::string GetClassName() { return std::string(#typ); }                                  \
  static int  ClassType;

enum ObserverEvent
{
  Deleted, Updated
};

class GalaxyObject
{
public:
  static GalaxyObjectP Cast(GalaxyObjectP kop) { return kop; }
  static bool IsA(GalaxyObjectP a) { return true; }
  static bool IsA(GalaxyObject* a) { return true; }
  std::string GetClassName() { return std::string("GalaxyObject"); }

  GalaxyObject() {}
  ~GalaxyObject()
  {
    NotifyObservers(Deleted, NULL);
    for (auto o : observed)
      o->UnregisterObserver(this);
  }

  void Observe(GalaxyObjectP o) { Observe(o.get()); }
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

  void Unobserve(GalaxyObjectP o) { Unobserve(o.get()); }
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
    
  void Notify(GalaxyObjectP o, ObserverEvent id, void *cargo) { Notify(o.get(), id, cargo); }

  void RegisterObserver(GalaxyObjectP o) { RegisterObserver(o.get()); }
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

}
