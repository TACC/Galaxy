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


#include "ObjectFactory.h"

using namespace std;

namespace gxy
{

static int obj_count = 0;
int get_obj_count() { return obj_count; }
void inc_obj_count() { obj_count++; }
void dec_obj_count() { obj_count--; }

int
ObjectFactory::register_class(GalaxyObject *(*n)(), std::string s)
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

GalaxyObjectPtr
ObjectFactory::New(GalaxyObjectClass c)
{
  return std::shared_ptr<GalaxyObject>(new_procs[c]());
}

static ObjectFactory theObjectFactory;
ObjectFactory *GetTheObjectFactory() { return &theObjectFactory; }

} // namespace gxy
