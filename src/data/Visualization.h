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


#include "Box.h"
#include "Datasets.h"
#include "KeyedDataObject.h"
#include "KeyedObject.h"
#include "Lighting.h"
#include "MappedVis.h"
#include "ParticlesVis.h"
#include "TrianglesVis.h"
#include "VolumeVis.h"

#include <ospray/ospray.h>

#include <map>

namespace gxy
{

KEYED_OBJECT_POINTER(Visualization)

class Visualization : public KeyedObject, public ISPCObject
{
  KEYED_OBJECT(Visualization);

  using vis_t = std::vector<VisP>;

public:
  void test();

  virtual ~Visualization();
  virtual void initialize();

  static std::vector<VisualizationP> LoadVisualizationsFromJSON(rapidjson::Value&);

  virtual void Commit(DatasetsP);

  virtual bool local_commit(MPI_Comm);

  virtual void Drop();

  void AddOsprayGeometryVis(VisP g);
  void AddMappedGeometryVis(VisP m);
  void AddVolumeVis(VisP v);

  virtual void SaveToJSON(rapidjson::Value&, rapidjson::Document&);
  void LoadFromJSON(rapidjson::Value&);

  void  SetAnnotation(std::string a) { annotation = a; }
  const char *GetAnnotation() { return annotation.c_str(); }

  OSPModel GetTheModel() { return ospModel; }

  Box *get_global_box() { return &global_box; }
  Box *get_local_box() { return &local_box; }

  int get_neighbor(int i) { return neighbors[i]; }
  bool has_neighbor(unsigned int face) { return neighbors[face] >= 0; }

	Lighting *get_the_lights() { return &lighting; }

protected:
	Lighting lighting;

  virtual void allocate_ispc();
  virtual void initialize_ispc();
  virtual void destroy_ispc();

  OSPModel ospModel;

  virtual int serialSize();
  virtual unsigned char *serialize(unsigned char *);
  virtual unsigned char *deserialize(unsigned char *);

  std::string annotation;

  vis_t osprayGeometries;
  vis_t mappedGeometries;
  vis_t volumes;

  Box global_box;
  Box local_box;
  int neighbors[6];
};

} // namespace gxy
