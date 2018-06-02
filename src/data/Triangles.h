#pragma once

#include <string>
#include <string.h>
#include <memory.h>

using namespace std;

#include "KeyedDataObject.h"
#include "dtypes.h"
#include "Application.h"
#include "Box.h"
#include "Geometry.h"
#include "Datasets.h"
using namespace std;

#include "rapidjson/document.h"

#include "ospray/ospray.h"

namespace pvol
{

KEYED_OBJECT_POINTER(Triangles)

class Triangles : public Geometry
{
  KEYED_OBJECT_SUBCLASS(Triangles, Geometry)

public:
	void initialize();
	virtual ~Triangles();

  virtual bool local_commit(MPI_Comm);
  virtual void local_import(char *, MPI_Comm);

  virtual void LoadFromJSON(Value&);
  virtual void SaveToJSON(Value&, Document&);

private:
	int n_triangles;
	int n_vertices;
	float *vertices;
	float *normals;
	int *triangles;
};

}
