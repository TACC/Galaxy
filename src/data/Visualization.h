#pragma once

#include <ospray/ospray.h>
        
#include <vector>
#include <string>
#include <memory>
#include <string.h>


#include "KeyedObject.h"
#include "KeyedDataObject.h"
#include <map>

using namespace std;


#include "Box.h"
#include "Datasets.h"
#include "VolumeVis.h"
#include "ParticlesVis.h"
#include "TrianglesVis.h"
#include "MappedVis.h"

namespace pvol
{

KEYED_OBJECT_POINTER(Visualization)

class Visualization : public KeyedObject, public ISPCObject
{
  KEYED_OBJECT(Visualization);

  using vis_t = vector<VisP>;

public:
  void test();

  virtual ~Visualization();
  virtual void initialize();

  static vector<VisualizationP> LoadVisualizationsFromJSON(Value&);

  virtual void Commit(DatasetsP);

  virtual bool local_commit(MPI_Comm);

  virtual void Drop();

  void AddOsprayGeometryVis(VisP g);
  void AddMappedGeometryVis(VisP m);
  void AddVolumeVis(VisP v);

  virtual void SaveToJSON(Value&, Document&);
  void LoadFromJSON(Value&);

  void  SetAnnotation(string a) { annotation = a; }
  const char *GetAnnotation() { return annotation.c_str(); }

  OSPModel GetTheModel() { return ospModel; }

  Box *get_global_box() { return &global_box; }
  Box *get_local_box() { return &local_box; }

  int get_neighbor(int i) { return neighbors[i]; }
  bool has_neighbor(unsigned int face) { return neighbors[face] >= 0; }

protected:
  virtual void allocate_ispc();
  virtual void initialize_ispc();
  virtual void destroy_ispc();

  OSPModel ospModel;

  virtual int serialSize();
  virtual unsigned char *serialize(unsigned char *);
  virtual unsigned char *deserialize(unsigned char *);

  string annotation;

  vis_t osprayGeometries;
  vis_t mappedGeometries;
  vis_t volumes;

  Box global_box;
  Box local_box;
  int neighbors[6];
};

}
