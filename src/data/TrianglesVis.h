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
#include "Triangles.h"
#include "Datasets.h"

namespace pvol
{

KEYED_OBJECT_POINTER(TrianglesVis)

class TrianglesVis : public Vis
{
  KEYED_OBJECT_SUBCLASS(TrianglesVis, Vis) 

public:
	~TrianglesVis();
  
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
