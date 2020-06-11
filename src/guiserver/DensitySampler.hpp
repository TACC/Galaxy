// ========================================================================== //
//                                                                            //
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

#include <iostream>
#include <sstream>

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>

#include <dtypes.h>

#include "rapidjson/document.h"

#include "Volume.h"
#include "Particles.h"
#include "Datasets.h"
#include "Filter.h"

#include <time.h>

using namespace gxy;
using namespace std;

#define RNDM ((float)rand() / RAND_MAX) 

namespace gxy
{


struct dMsg : public filterArgs
{
  int nSamples;
  float power;
};

class DSamplerMsg : public Work
{

public:
  DSamplerMsg(dMsg* a) : DSamplerMsg(sizeof(dMsg))
  {
    *(dMsg *)contents->get() = *a;
  }

  WORK_CLASS(DSamplerMsg, true);

  bool CollectiveAction(MPI_Comm, bool);

  float *
  CellValues(VolumeP v)
  {
    // Note: the 3 here is the point count of the partition 
    // minus 2, for the ghost points, minus 1 for the point-to-cell
    // conversion

    int nCells = (gni-3) * (gnj-3) * (gnk-3);
    float *cellValues = new float[nCells];

    float *d = cellValues;
    if (v->get_type() == Volume::FLOAT)
    {
      float *s = (float *)v->get_samples();
      for (int i = 1; i < gni-2; i++)
        for (int j = 1; j < gnj-2; j++)
          for (int k = 1; k < gnk-2; k++)
          {
            int corner = i*istep + j*jstep + k*kstep;
            *d++ = s[corner + 0*istep + 0*jstep + 0*kstep] +
                   s[corner + 1*istep + 0*jstep + 0*kstep] +
                   s[corner + 0*istep + 1*jstep + 0*kstep] +
                   s[corner + 1*istep + 1*jstep + 0*kstep] +
                   s[corner + 0*istep + 0*jstep + 1*kstep] +
                   s[corner + 1*istep + 0*jstep + 1*kstep] +
                   s[corner + 0*istep + 1*jstep + 1*kstep] +
                   s[corner + 1*istep + 1*jstep + 1*kstep];
          }
    }
    else
    {
      double *s = (double *)v->get_samples();
      for (int i = 1; i < gni-2; i++)
        for (int j = 1; j < gnj-2; j++)
          for (int k = 1; k < gnk-2; k++)
          {
            int corner = i*istep + j*jstep + k*kstep;
            *d++ = s[corner + 0*istep + 0*jstep + 0*kstep] +
                   s[corner + 1*istep + 0*jstep + 0*kstep] +
                   s[corner + 0*istep + 1*jstep + 0*kstep] +
                   s[corner + 1*istep + 1*jstep + 0*kstep] +
                   s[corner + 0*istep + 0*jstep + 1*kstep] +
                   s[corner + 1*istep + 0*jstep + 1*kstep] +
                   s[corner + 0*istep + 1*jstep + 1*kstep] +
                   s[corner + 1*istep + 1*jstep + 1*kstep];
          }
    }

    return cellValues;
  }

  void
  Sample(MPI_Comm c, dMsg *a)
  {
    VolumeP v = Volume::Cast(KeyedDataObject::GetByKey(a->sourceKey));
    ParticlesP p = Particles::Cast(KeyedDataObject::GetByKey(a->destinationKey));

    p->setModified(true);

    nSamples = a->nSamples;
    power = a->power;
  
    p->clear();
    p->CopyPartitioning(v);
    p->SetDefaultColor(1.0, 1.0, 1.0, 1.0);

    v->get_ghosted_local_counts(gni, gnj, gnk); // absolute counts, inc. ghosts
    v->get_local_origin(ox, oy, oz);
    v->get_deltas(dx, dy, dz);

    istep = 1;
    jstep = gni;
    kstep = gni * gnj;

    float *cellValues = CellValues(v);

    // Note: as above, the 3 here is the point count of the partition 
    // minus 2, for the ghost points, minus 1 for the point-to-cell
    // conversion

    int nCells = (gni-3) * (gnj - 3) * (gnk - 3);

    float local_min = cellValues[0], local_max = cellValues[0];
    for (int i = 0; i < nCells; i++)
    {
      float c = cellValues[i];
      if (c < local_min) local_min = c;
      if (c > local_max) local_max = c;
    }
  
    float global_min, global_max;

    MPI_Allreduce(&local_min, &global_min, 1, MPI_FLOAT, MPI_MIN, c);
    MPI_Allreduce(&local_max, &global_max, 1, MPI_FLOAT, MPI_MAX, c);

    float local_total = 0;
    if (global_min != global_max)
    {
      for (int i = 0; i < nCells; i++)
      {
        cellValues[i] = pow((cellValues[i] - global_min) / (global_max - global_min), power);
        local_total += cellValues[i];
      }
    }
    else
    {
      local_total = nCells;
      for (int i = 0; i < nCells; i++)
        cellValues[i] = 1.0;
    }

    float global_total;
    MPI_Allreduce(&local_total, &global_total, 1, MPI_FLOAT, MPI_SUM, c);

    float f_this_part = (local_total / global_total) * nSamples;
    int n_this_part   = ((int)f_this_part) + ((RNDM < (f_this_part - (int)f_this_part)) ? 1 : 0);

    float *tcell = cellValues;

    // Note: local_total can be zero if the partition takes on the minimum
    // value everywhere.  This means there will be no samples in this partition.

    if (local_total > 0)
    {
      // Note: gni, gnj, and gnk are the number of points along
      // each axis including ghosts.   The following loop is
      // through *cells*.   So... if the vertices are
      // 
      // a b c d e f g
      //
      // then g is 7.  The cells at a and f are ghosts, so the
      // non-ghost cells start at b, c, d, e which
      // is index 1, 2, 3, 4 or i = 1; i < (g - 2)

      for (int i = 0; i < gni-3; i++)
      {
        float x = ox + i*dx;

        for (int j = 0; j < gnj-3; j++)
        {
          float y = oy + j*dy;

          for (int k = 0; k < gnk-3; k++)
          {
            float z = oz + k*dz;

            float f_this_cell = (*tcell++ / local_total) * f_this_part;
            int n_this_cell = ((int)f_this_cell) + ((RNDM < (f_this_cell - (int)f_this_cell)) ? 1 : 0);

            for (int l = 0; l < n_this_cell; l++)
            {
              Particle s(x + RNDM*dx, y + RNDM*dy, z + RNDM*dz, 0);
              p->push_back(s);
            }
          }
        }
      }
    }

    std::cerr << "created " << p->GetNumberOfVertices() << " samples\n";
  }

private:
  int   nSamples;
  float power;
  int   gni,  gnj,  gnk;      // ghosted counts along each local axis
  int   istep, jstep, kstep;  // strides in ghosted space
  float ox, oy, oz;           // local grid origin
  float dx, dy, dz;           // grid step size
};

class DensitySampler : public Filter
{
  static int densityfilter_index;

public:

  static void
  init()
  {
    srand(GetTheApplication()->GetRank());
    DSamplerMsg::Register();
  }

  DensitySampler(KeyedDataObjectP source)
  {
    std::stringstream ss;
    ss << "DensityFilter_" << densityfilter_index++;
    name = ss.str();

    result = KeyedDataObject::Cast(Particles::NewP());
    result->CopyPartitioning(source);
  }

  ~DensitySampler() {}

  void
  Sample(rapidjson::Document& doc, KeyedDataObjectP scalarVolume)
  {
    dMsg args;

    args.sourceKey      = scalarVolume->getkey();
    args.destinationKey = result->getkey();

    args.nSamples       = doc["nSamples"].GetInt();
    args.power          = doc["power"].GetInt();

    DSamplerMsg msg(&args);
    msg.Broadcast(true, true);

    result->Commit();
  }
};

}
