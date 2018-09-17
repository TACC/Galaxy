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

/*! \file AmrVolume.h
 * \brief an AMR volume dataset within Galaxy. 
 * \ingroup data
 */

#include <Volume.h>
#include <ospray/ospray.h>

namespace gxy
{
KEYED_OBJECT_POINTER(AmrVolume)

//! an AMR volumetric dataset within Galaxy. This class encapsulates an ospray 
//  AMR volume object and makes it available to Galaxy
/*! \ingroup data
 *  \sa Volume, KeyedObject, KeyedDataObject, OSPRayObject
 */
class AmrVolume : public Volume
{
    KEYED_OBJECT_SUBCLASS( AmrVolume,Volume)
        
public:
        virtual void initialize(); //!< initialize this AMR Volume
        virtual ~AmrVolume(); //!< default destructor

        //! commit this object to the local registry
        /*! This action is performed in response to a CommitMsg */
        virtual bool local_commit(MPI_Comm);
        //! import the given data file into local memory
        /*! This action is performed in response to a ImportMsg */
        virtual void local_import(char *fname, MPI_Comm c);
        //! load a timestep into local memory
        /*! This action is performed in response to LoadTimestepMsg */
        virtual bool local_load_tiestep(MPI_Comm c);


};
} // namespace gxy
