// ========================================================================== //
// // Copyright (c) 2014-2018 The University of Texas at Austin.                 //
// // All rights reserved.                                                       //
// //                                                                            //
// // Licensed under the Apache License, Version 2.0 (the "License");            //
// // you may not use this file except in compliance with the License.           //
// // A copy of the License is included with this software in the file LICENSE.  //
// // If your copy does not contain the License, you may obtain a copy of the    //
// // License at:                                                                //
// //                                                                            //
// //     https://www.apache.org/licenses/LICENSE-2.0                            //
// //                                                                            //
// // Unless required by applicable law or agreed to in writing, software        //
// // distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  //
// // WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           //
// // See the License for the specific language governing permissions and        //
// // limitations under the License.                                             //
// //                                                                            //
// // ========================================================================== //

#include <iostream>

#include "Application.h"
#include "AmrVolume.h"
#include "Datasets.h"

namespace gxy
{
KEYED_OBJECT_TYPE(AmrVolume)

void
AmrVolume::initialize()
{
    super::initialize();
}

AmrVolume::~AmrVolume()
{
    // put stuff here
}

bool
AmrVolume::local_commit(MPI_Comm c)
{
    OSPRayObject::local_commit(c);
    OSPVolume ospv = ospNewVolume("amr_volume");
    // these two vectors hold the topology and data
    // of each grid respectively.
    std::vector<OSPData> brickDataArray;
    std::vector<ospray::amr::amr_brick_info> brickInfoArray;
    // Traverse the AMR Levels 
    for(level : ) 
    {
        fltptr = getScalarData();
        osp::vec3i counts;
        get_ghosted_local_counts(level,counts.x,counts.y,counts.z); // add this
        osp::vec3f spacing;
        get_deltas(level,spacing.x,spacing.y,spacing.z); // and this
        OSPData odata = ospNewData(counts.x*counts.y*counts.z,
            get_type() == FLOAT?OSP_FLOAT:OSP_UCHAR,fltptr,OSP_DATA_SHARED_BUFFER);
        ospCommit(odata);
        brickDataArray.push_back(odata);
        ospcommon::box3i obox={lo_v,hi_v};
        amr_brick_info bi;
        bi.bounds = obox;
        bi.cellWidth = spacing[0];
        bi.refinementLevel = level;
        brickInfoArray.push_back(bi)
    }
    ospSet1f(ospv,"samplingRate",this->samplingrate);
    osp::vec3f origin;
    get_ghosted_local_origin(origin.x,origin.y,origin.z);
    ospSetVec3f(ospv,"gridOrigin",origin);
    OSPData brickDataData=ospNewData(brickDataArray.size(),
            OSP_OBJECT,&brickDataArray[0],0);
    ospCommit(brickDataData);
    ospSetData(ospv,"brickData",brickDataData);
    OSPData brickInfoData = ospNewData(brickInfoArray.size()*sizeof(brickInfoArray[0])
            ,OSP_RAW,&brickInfoArray[0],0);
    ospCommit(brickInfoData);
    ospSetData(ospv,"brickInfo",brickInfoData);
    ospSetObject(ospv,"transferFunction",ospNewTransferFunction("piecewise_linear"));
    ospCommit(ospv);
    if(theOSPRayObject)
        ospRelease(theOSPRayObject);
    theOSPRayObject = ospv;
    return false;
}
void
AmrVolume::local_import(char *fname, MPI_Comm c)
{
    string type_string,data_fname;
    filename = fname;

    int rank = GetTheApplication()->GetRank();
    int size = GetTheApplication()->GetSize();
    // here we examine the fname to determine, from extension, what type
    // of data we are looking at and read it appropriately.
    
    // dir contains the directory of the file or empty string
    string dir((filename.find_last_of("/") == string::npos) ? "" : 
            filename.substr(0,filename.find_last_of("/")+1));

    string ext((filename.find_last_of(".") == string::npos) ? "" :
            filename.substr(filename.find_last_of(".")+1,filename.length()));
    switch(ext) 
    {
        case "amrvol": readgxyamrheader();
            break;
        case "hierarchy": readEnzoHierarchy();
            break;
        case default:
            std::cerr << "indeterminate input file type" << std::endl;
    }
            
}

} // namespace gxy
