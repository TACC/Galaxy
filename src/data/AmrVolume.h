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

#include <vtkStructuredPointsReader.h>
#include <vtkStructuredPoints.h>

namespace gxy
{
OBJECT_POINTER_TYPES(AmrVolume)

//! an AMR volumetric dataset within Galaxy. This class encapsulates an ospray 
//  AMR volume object and makes it available to Galaxy
/*! \ingroup data
 *  \sa Volume, KeyedObject, KeyedDataObject
 */
class AmrVolume : public Volume
{
    KEYED_OBJECT_SUBCLASS( AmrVolume,Volume)
        
public:
        virtual void initialize(); //!< initialize this AMR Volume
        virtual ~AmrVolume(); //!< default destructor

        //! commit this object to the local registry
        /*! This action is performed in response to a ImportMsg */
        virtual void local_import(char *fname, MPI_Comm c);
        //! load a timestep into local memory
        /*! This action is performed in response to LoadTimestepMsg */
        virtual bool local_load_timestep(MPI_Comm c);
        //! method to read gxyamr data
        /*! an internal method used to load gxyamr metadata */

private:
        enum amrtype {GXYAMR,ENZOAMR,NOEXT};
        //! method to read gxyamr data
        /*! an internal method used to load gxyamr metadata */
        void ReadGxyAmrHeader();
        //! method to read Enzo data
        /*! an internal method used to load Enzo Hierarchy metadata */
        void ReadEnzoHierarchy();
        int get_grid_index(int level, int grid);
        float *getScalarData(int level, int grid);
        void get_counts(int level,int grid, int& cx,int& cy,int& cz);
        vec3i get_counts(int level,int grid);
        vec3f get_origin(int level,int grid);
        void get_origin(int level, int grid, float& x,float& y, float& z);
        void get_deltas(int level, int grid, float& sx, float& sy, float& sz);
        void set_counts(int level, int grid, vec3i& counts);
        void set_deltas(int level, int grid, vec3f& deltas);



protected:
        int numlevels;
        int numgrids;
        std::vector<int> levelnumgrids;
        std::vector<std::string> gridfilenames;
        float samplingRate;
        std::vector<vec3f> gridorigin;
        std::vector<vec3f> gridspacing;
        std::vector<vec3i> gridcounts;
        std::vector<float*> gridsamples;
};
} // namespace gxy
