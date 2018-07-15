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

#include <string>
#include <string.h>
#include <vector>
#include <memory>


#include "Datasets.h"
#include "dtypes.h"
#include "ISPCObject.h"
#include "KeyedObject.h"
#include "OSPRayObject.h"

namespace gxy
{

KEYED_OBJECT_POINTER(Vis)
  
class Vis : public KeyedObject, public ISPCObject
{
    KEYED_OBJECT(Vis)

public:
    virtual ~Vis();

    virtual void Commit(DatasetsP);
    OSPRayObjectP GetTheData() { return data; }
    void SetTheData( OSPRayObjectP d ) { data = d; }

    virtual void LoadFromJSON(rapidjson::Value&);
    virtual void SaveToJSON(rapidjson::Value&, rapidjson::Document&);
    void SetName(std::string n) { name = n; }
    virtual bool local_commit(MPI_Comm);

protected:
    virtual void allocate_ispc();
    virtual void initialize_ispc();
    virtual void destroy_ispc();

    virtual int serialSize();
    virtual unsigned char *serialize(unsigned char *);
    virtual unsigned char *deserialize(unsigned char *);

    std::string name;
    Key datakey;
    OSPRayObjectP data;
};

} // namespace gxy
