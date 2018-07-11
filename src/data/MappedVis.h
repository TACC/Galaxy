#pragma once

#include <string>
#include <string.h>
#include <vector>
#include <memory>

#include "ospray/ospray.h"

#include "dtypes.h"
#include "Vis.h"
#include "Datasets.h"
#include "KeyedObject.h"

namespace gxy
{
KEYED_OBJECT_POINTER(MappedVis)

class MappedVis : public Vis
{
  KEYED_OBJECT_SUBCLASS(MappedVis, Vis)

public:

  virtual ~MappedVis();

  virtual void initialize();

  virtual void Commit(DatasetsP);

  void SetColorMap(int, vec4f *);
  void SetOpacityMap(int, vec2f *);

  virtual void LoadFromJSON(Value&);
  virtual void SaveToJSON(Value&, Document&);

  virtual bool local_commit(MPI_Comm);

 protected:
  virtual void allocate_ispc();
  virtual void initialize_ispc();
 
  vector<vec4f> colormap;
  vector<vec2f> opacitymap;

  virtual int serialSize();
  virtual unsigned char *serialize(unsigned char *);
  virtual unsigned char *deserialize(unsigned char *);

  OSPTransferFunction transferFunction;
  
};
}

