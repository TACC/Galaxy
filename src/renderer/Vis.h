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

/*! \file Vis.h 
 * \brief the parent class for visualization elements that operate on a dataset within Galaxy
 * \ingroup data
 */

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

OBJECT_POINTER_TYPES(Vis)
  
//! the parent class for visualization elements that operate on a dataset within Galaxy
/* This object represents a single visualization element, namely a set of filters applied to data. 
 * In contrast, a Visualization object combines one or more Vis objects with
 * with lighting information.
 * \ingroup data 
 * \sa Visualization, KeyedObject, ISPCObject, OSPRayObject
 */
class Vis : public KeyedObject, public ISPCObject
{
    KEYED_OBJECT(Vis)

public:
    virtual ~Vis(); //!< default destructor

    //! commit this object to the global registry across all processes
    virtual void Commit(DatasetsP);

    //! return a pointer to the KeyedDataObject data that this Vis targets
    KeyedDataObjectP GetTheData() { return data; }

    //! set the KeyedDataObject data that this Vis should target
    void SetTheData( KeyedDataObjectP d ) { data = d; }

    //! set the OSPRayObject for this Vis' data   This is called when we start rendering a RenderingSet.
    //! 
    void SetTheOSPRayDataObject(OSPRayObjectP o);

    //! construct a Vis from a Galaxy JSON specification
    virtual void LoadFromJSON(rapidjson::Value&);
    //! save this Vis to a Galaxy JSON specification 
    virtual void SaveToJSON(rapidjson::Value&, rapidjson::Document&);

    //! set the name of this Vis
    void SetName(std::string n) { name = n; }

    //! commit this object to the local registry
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
    KeyedDataObjectP data;
};

} // namespace gxy
