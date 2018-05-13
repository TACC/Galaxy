#pragma once

#include <string>
#include <string.h>
#include <vector>
#include <memory>

using namespace std;

#include "KeyedObject.h"


#include "dtypes.h"
#include "Application.h"
#include "Vis.h"
#include "Particles.h"
#include "Datasets.h"

namespace pvol
{

KEYED_OBJECT_POINTER(ParticlesVis)

class ParticlesVis : public Vis
{
  KEYED_OBJECT_SUBCLASS(ParticlesVis, Vis) 

public:
	~ParticlesVis();
  
  virtual void initialize();
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
};

}
