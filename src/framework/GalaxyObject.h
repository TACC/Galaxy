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

#include <memory>
#include <string>

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
  std::string GetClassName() { return std::string(#typ); }                                  \
  static int  ClassType;


class GalaxyObject
{
public:
  GalaxyObject() {}

  //! initialize this object (default has no action)
  virtual void initialize() { error = 0; }

  int get_error() { return error; }
  void set_error(int e) { error = e; }

protected:
  int error;
};

#define GALAXY_OBJECT(typ)  GALAXY_OBJECT_SUBCLASS(typ, GalaxyObject)


}
