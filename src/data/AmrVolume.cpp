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
#include <cmath>

#include "Application.h"
#include "AmrVolume.h"

namespace gxy
{
KEYED_OBJECT_CLASS_TYPE(AmrVolume)

void
AmrVolume::initialize()
{
    this->samplingRate = 0.5;
    super::initialize();
}

AmrVolume::~AmrVolume()
{
    // put stuff here
}

void
AmrVolume::Register()
{
    RegisterClass();
}

// this is the same factor function taken from Volume.cpp. Declared as static there.
// this version may take into account amr nature of the data

static void
factor(int ijk, vec3i &factors)
{
//  If ijk is prime, 1, 1, ijk will be chosen, i+j+k == ijk+2

 if (ijk == 1)
 {
   factors.x = factors.y = factors.z = 1;
   return;
 }

 int mm = ijk+3;
 for (int i = 1; i <= ijk>>1; i++)
 {
   int jk = ijk / i;
   if (ijk == (i * jk))
   {
      for (int j = 1; j <= jk>>1; j++)
       {
          int k = jk / j;
          if (jk == (j * k))
          {
            int m = i + j + k;
            if (m < mm)
            {
               mm = m;
               factors.x = i;
               factors.y = j;
               factors.z = k;
             }
           }
       }
    }
 }
}

// this struct also appears in Volume.cpp. It needs to be placed in
// a header file for all to enjoy..
struct part
{
    vec3i ijk;
    vec3i counts;
    vec3i gcounts;
    vec3i offsets;
    vec3i goffsets;
};

/*part *
partition(int ijk, vec3i factors, vec3i grid)
{
  int ni = grid.x - 2;
  int nj = grid.y - 2;
  int nk = grid.z - 2;

  int di = ni / factors.x;
  int dj = nj / factors.y;
  int dk = nk / factors.z;

  part *parts = new part[ijk];
  part *p = parts;
  for (int k = 0; k < factors.z; k++)
    for (int j = 0; j < factors.y; j++)
      for (int i = 0; i < factors.x; i++, p++)
      {
        p->ijk.x = i;
        p->ijk.y = j;
        p->ijk.z = k;

        p->offsets.x = 1 + i*di;
        p->offsets.y = 1 + j*dj;
        p->offsets.z = 1 + k*dk;

        p->goffsets.x = p->offsets.x - 1;
        p->goffsets.y = p->offsets.y - 1;
        p->goffsets.z = p->offsets.z - 1;

        p->counts.x = 1 + ((i == (factors.x-1)) ? ni - p->offsets.x : di);
        p->counts.y = 1 + ((j == (factors.y-1)) ? nj - p->offsets.y : dj);
        p->counts.z = 1 + ((k == (factors.z-1)) ? nk - p->offsets.z : dk);

        p->gcounts.x = p->counts.x + 2;
        p->gcounts.y = p->counts.y + 2;
        p->gcounts.z = p->counts.z + 2;
      }

    return parts;
}*/
extern part*
partition(int ijk, vec3i factors, vec3i grid);

void
AmrVolume::local_import(char *fname, MPI_Comm c)
{
    std::string type_string,data_fname;
    filename = fname;
    std::cerr << "AMR local import" << std::endl;
    vtkStructuredPointsReader *gridreader = vtkStructuredPointsReader::New();
    vtkStructuredPoints *gsp;
    int rank = GetTheApplication()->GetRank();
    int size = GetTheApplication()->GetSize();
    // here we examine the fname to determine, from extension, what type
    // of data we are looking at and read it appropriately.
    
    // dir contains the directory of the file or empty string
    std::string dir((filename.find_last_of("/") == std::string::npos) ? "" : 
            filename.substr(0,filename.find_last_of("/")+1));

    std::string ext((filename.find_last_of(".") == std::string::npos) ? "" :
            filename.substr(filename.find_last_of(".")+1,filename.length()));
    amrtype flt = NOEXT;
    // do something more clever later.
    if(ext == "amrvol") 
    {
        flt = GXYAMR;
    } 
    else if(ext == "hierarchy") 
    {
        flt = ENZOAMR;
    } 
    switch(flt) 
    {
        case GXYAMR: ReadGxyAmrHeader();
            break;
        case ENZOAMR: ReadEnzoHierarchy();
            break;
        default: std::cerr << "indeterminate input file type" << std::endl;
            break;
    }
    // now read the level 0 grid and decompose it (if necessary) as done
    // in the parent class. There can be many level 0 grids. We need to 
    // move this into the loop over grids and levels below. 
    gridreader->SetFileName(gridfilenames[0].c_str());
    gsp = gridreader->GetOutput();
    gridreader->Update();
    // now get some metadata
    // The level 0 grid loads "global metadata" as well as gridspacing and 
    // gridcounts vectors. (data is duplicated)
    double dvector[3],range[2],bounds[6];
    int ivector[3];
    float *fltptr;
    vec3f tmpvec,lowerbound,upperbound;
    gsp->GetOrigin(dvector);
    gsp->ComputeBounds();
    gsp->GetBounds(bounds);
    lowerbound.x = bounds[0];
    lowerbound.y = bounds[2];
    lowerbound.z = bounds[4];
    upperbound.x = bounds[1];
    upperbound.y = bounds[3];
    upperbound.z = bounds[5];
    std::cerr <<"level0 lb" <<  lowerbound.x << " " << lowerbound.y << " " << lowerbound.z << std::endl;
    std::cerr <<"level0 ub" <<  upperbound.x << " " << upperbound.y << " " << upperbound.z << std::endl;
    std::cerr << "AMRV: origin " << dvector[0] << " " << dvector[1] <<" " << dvector[2] << std::endl;
    global_origin.x = dvector[0];
    global_origin.y = dvector[1];
    global_origin.z = dvector[2];
    gridorigin.push_back(global_origin);
    gsp->GetDimensions(ivector);
    std::cerr << "AMRV: dimensions " << ivector[0] << " " << ivector[1] <<" " << ivector[2] << std::endl;
    global_counts.x = ivector[0] -1;
    global_counts.y = ivector[1] -1;
    global_counts.z = ivector[2] -1;
    // save the data to gridcounts vector too. level= grid = 0..
    gridcounts.push_back(global_counts);
    gsp->GetSpacing(dvector);
    std::cerr << "AMRV: spacing " << dvector[0] << " " << dvector[1] <<" " << dvector[2] << std::endl;
    deltas.x = dvector[0];
    deltas.y = dvector[1];
    deltas.z = dvector[2];
    // save the data to gridspacing
    gridspacing.push_back(deltas);
    // now partition the level 0 grid if needed..
    vec3i global_partitions;
    factor(size,global_partitions);
//    std::cerr << "AMRV: part " << global_partitions.x << " " 
//        << global_partitions.y << " " 
//        << global_partitions.z << std::endl;
    part *partitions = partition(size,global_partitions, global_counts);
 //   std::cerr << " AMRV: rank " << rank << std::endl;
    part *my_partition = partitions + rank;
    local_offset = my_partition->offsets;
    local_counts = my_partition->counts;
    ghosted_local_offset = my_partition->goffsets;
    ghosted_local_counts = my_partition->gcounts;
    // done partitioning level0 metadata. Use it to get some scalars
    // NOTE: this is broke using vtk datasets.
    // just set the level0 data to point to all of level 0 for now.
    // insert malloc of space for points here and copy gsp data to new location
    int npts = gsp->GetNumberOfPoints();
    std::cerr << " level0 grid has " << npts << " points" << std::endl;
    if(gsp->GetScalarType() == VTK_FLOAT)
        std::cerr << " scalars type float" << std::endl;
    else
        std::cerr << " scalars type not flota " << std::endl;
    type = gsp->GetScalarType() == VTK_FLOAT?FLOAT:UCHAR;
    fltptr = new float[npts];
    float* dptr = (float*)gsp->GetScalarPointer();
    //gridsamples.push_back((float*)gsp->GetScalarPointer());
    for(int p = 0; p<npts;p++) fltptr[p] = dptr[p];
    gridsamples.push_back(fltptr);
    float* yourmama = (float*)gsp->GetScalarPointer();
    std::cerr << "first float from gsp" << yourmama[0]<< " " << yourmama[1] << std::endl;
    gsp->GetScalarRange(range);
    local_min = range[0];
    local_max = range[1];
    // now do the rest of the grids in all the levels
    for(int l=1;l<numlevels;l++)
    {
        // get all the grids in this level
        for(int g=0;g<levelnumgrids[l];g++)
        {
            int index = get_grid_index(l,g);
            gridreader->SetFileName(gridfilenames[index].c_str());
            gridreader->Update();
            gsp->GetOrigin(dvector);
            gsp->GetScalarRange(range);
            local_min = (local_min < range[0])?local_min:range[0];
            local_max = (local_max > range[1])?local_max:range[1];
            tmpvec.x = dvector[0];
            tmpvec.y = dvector[1];
            tmpvec.z = dvector[2];
            gridorigin.push_back(tmpvec);
            gsp->GetDimensions(ivector);
            gridcounts.push_back(vec3i(ivector[0]-1,ivector[1]-1,ivector[2]-1));
            gsp->GetSpacing(dvector);
            tmpvec.x = dvector[0];
            tmpvec.y = dvector[1];
            tmpvec.z = dvector[2];
            gridspacing.push_back(tmpvec);
            dptr = (float*)gsp->GetScalarPointer();
            npts = gsp->GetNumberOfPoints();
            fltptr = new float[npts];
            for(int p = 0; p<npts;p++) fltptr[p] = dptr[p];
            gridsamples.push_back(fltptr);
            //gridsamples.push_back((float*)gsp->GetScalarPointer());
        }
    }
    // a macro from the volume object. 
#define ijk2rank(i, j, k) ((i) + ((j) * global_partitions.x) + ((k) * global_partitions.x * global_partitions.y))
    // set neighbors
    neighbors[0] = (my_partition->ijk.x > 0) ? ijk2rank(my_partition->ijk.x - 1, my_partition->ijk.y, my_partition->ijk.z) : -1;
    neighbors[1] = (my_partition->ijk.x < (global_partitions.x-1)) ? ijk2rank(my_partition->ijk.x + 1, my_partition->ijk.y, my_partition->ijk.z) : -1;
    neighbors[2] = (my_partition->ijk.y > 0) ? ijk2rank(my_partition->ijk.x, my_partition->ijk.y - 1, my_partition->ijk.z) : -1;
    neighbors[3] = (my_partition->ijk.y < (global_partitions.y-1)) ? ijk2rank(my_partition->ijk.x, my_partition->ijk.y + 1, my_partition->ijk.z) : -1;
    neighbors[4] = (my_partition->ijk.z > 0) ? ijk2rank(my_partition->ijk.x, my_partition->ijk.y, my_partition->ijk.z - 1) : -1;
    neighbors[5] = (my_partition->ijk.z < (global_partitions.z-1)) ? ijk2rank(my_partition->ijk.x, my_partition->ijk.y, my_partition->ijk.z + 1) : -1;
    // set global values here. This wont hold up under mpi
    global_min = local_min;
    global_max = local_max;
    // We need to set the local_box. 
    set_local_box(lowerbound,upperbound);
    // global box same as local box in this one d example... bad juju. 
    global_box = Box(lowerbound,upperbound);


}
int
AmrVolume::get_grid_index(int level, int grid)
{
    int index = 0;
    for(int l = 0;l<level;l++) index = index + levelnumgrids[l];
    index += grid;
    return index;
}
void
AmrVolume::get_counts(int level, int grid, int& cx,int& cy,int& cz)
{
    int index;
    vec3i counts;
    index = get_grid_index(level,grid);
    counts = gridcounts[index];
    cx = counts.x;
    cy = counts.y;
    cz = counts.z;
}
vec3i
AmrVolume::get_counts(int level, int grid)
{
    int index;
    index = get_grid_index(level,grid);
    return gridcounts[index];
}
vec3f
AmrVolume::get_origin(int level, int grid)
{
    int index;
    index = get_grid_index(level,grid);
    return gridorigin[index];
}
void
AmrVolume::get_origin(int level,int grid,float& x, float& y, float& z)
{
   int index;
   vec3f sc;
   index = get_grid_index(level,grid);
   sc = gridorigin[index];
   x = sc.x;
   y = sc.y;
   z = sc.z;
}
void
AmrVolume::set_counts(int level, int grid, vec3i& counts) 
{
    int index;
    index = get_grid_index(level,grid);
    gridcounts[index] = counts;
}
void
AmrVolume::set_deltas(int level, int grid, vec3f& deltas)
{
    int index;
    index = get_grid_index(level,grid);
    gridspacing[index] = deltas;
}
void 
AmrVolume::get_deltas(int level, int grid, float& sx, float& sy, float& sz)
{
    int index;
    vec3f spacing;
    index = get_grid_index(level,grid);
    spacing = gridspacing[index];
    sx = spacing.x;
    sy = spacing.y;
    sz = spacing.z;
}
void
AmrVolume::ReadEnzoHierarchy()
{
}
float*
AmrVolume::getScalarData(int level, int grid)
{
    int index;
    index = get_grid_index(level,grid);
    return gridsamples[index];
}
void
AmrVolume::ReadGxyAmrHeader()
{
    ifstream in;
    in.open(filename.c_str());
    if (in.fail())
    {
        cerr<< "ERROR: unable to open volfile: " << filename << endl;
        exit(1);
    }
    in >> numlevels; // get the number of levels
    int grids;
    std::string gfn;
    numgrids = 0;
    for(int l = 0; l < numlevels; l++) // get the number of grids in each level
    {
        in >> grids;
        levelnumgrids.push_back(grids);   
        //in >> levelnumgrids[l];
        numgrids += levelnumgrids[l];
    }
    for(int i = 0; i<numgrids; i++)
    {
        in >> gfn;
        gridfilenames.push_back(gfn);
    }
}
bool
AmrVolume::local_load_timestep(MPI_Comm c) 
{
    return false;
}


} // namespace gxy
