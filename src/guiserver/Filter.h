
//                                                                            //
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
#include <vector>

#include "rapidjson/document.h"

#include "MultiServerHandler.h"
#include "Datasets.h"
#include "KeyedDataObject.h"

namespace gxy
{
  struct filterArgs
  {
    Key sourceKey;       
    Key destinationKey; 
  };

  class Filter
  {
  public:
    virtual ~Filter() {}

    static KeyedDataObjectP getSource(rapidjson::Document& doc)
    {
      std::string name = doc["source"].GetString();
      DatasetsP theDatasets = Datasets::Cast(MultiServer::Get()->GetGlobal("global datasets"));
      return theDatasets->Find(name);
    }

    void SetName(std::string n) { name = n; }
    std::string GetName() { return name; }

    KeyedDataObjectP getResult() { return result; }

  protected:
    KeyedDataObjectP result;
    std::string name;
  };
}
