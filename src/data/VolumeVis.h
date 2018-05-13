#pragma once

#include <string>
#include <string.h>
#include <vector>
#include <memory>

using namespace std;

#include "KeyedObject.h"


#include "dtypes.h"
#include "Application.h"

#include "Volume.h"
#include "MappedVis.h"
#include "Datasets.h"

namespace pvol
{

KEYED_OBJECT_POINTER(VolumeVis)


class VolumeVis : public MappedVis
{
  KEYED_OBJECT_SUBCLASS(VolumeVis, MappedVis) 

public:
	~VolumeVis();
  
  virtual void initialize();

  void AddSlice(vec4f s)
  {
    slices.push_back(s);
  }

  void SetSlices(int n, vec4f *s)
  {
    slices.clear();
    for (int i = 0; i < n; i++)
      AddSlice(s[i]);
  }
  
  void GetSlices(int& n, vec4f*& s)
  {
    n = slices.size();
    s = slices.data();
  }

  void AddIsovalue(float iv)
  {
    isovalues.push_back(iv);
  }

  void SetIsovalues(int n, float *isos)
  {
    isovalues.clear();
    for (int i = 0; i < n; i++)
      AddIsovalue(isos[i]);
  }
  
  void GetIsovalues(int& n, float*& i)
  {
    n = isovalues.size();
    i = isovalues.data();
  }

  void SetVolumeRendering(bool yn) { volume_rendering = yn; }
  bool GetVolumeRendering() { return volume_rendering; }

  virtual bool local_commit(MPI_Comm);

protected:

	virtual void initialize_ispc();
	virtual void allocate_ispc();
	virtual void destroy_ispc();

  virtual void LoadFromJSON(Value&);
  virtual void SaveToJSON(Value&, Document&);

  virtual int serialSize();
  virtual unsigned char* serialize(unsigned char *ptr);
  virtual unsigned char* deserialize(unsigned char *ptr);

  bool volume_rendering;

  vector<vec4f> slices;
  vector<float> isovalues;
};

}
