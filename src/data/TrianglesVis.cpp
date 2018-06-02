#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream>
#include "Application.h"
#include "TrianglesVis.h"
#include "Datasets.h"

#include "TrianglesVis_ispc.h"

namespace pvol
{

KEYED_OBJECT_TYPE(TrianglesVis)

void
TrianglesVis::Register()
{
  RegisterClass();
}

TrianglesVis::~TrianglesVis()
{
	TrianglesVis::destroy_ispc();
}

void
TrianglesVis::initialize()
{
  super::initialize();
}

void
TrianglesVis::initialize_ispc()
{
  super::initialize_ispc();
  ispc::TrianglesVis_initialize(ispc);
} 
    
void
TrianglesVis::allocate_ispc()
{
  ispc = ispc::TrianglesVis_allocate();
}

int 
TrianglesVis::serialSize()
{
  return super::serialSize();
}

unsigned char *
TrianglesVis::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);
  return ptr;
}

unsigned char *
TrianglesVis::deserialize(unsigned char *ptr)
{
  ptr = super::deserialize(ptr);
  return ptr;
}

void 
TrianglesVis::LoadFromJSON(Value& v)
{
  super::LoadFromJSON(v);
}

void
TrianglesVis::SaveToJSON(Value& v, Document&  doc)
{
  Vis::SaveToJSON(v, doc);
}

void
TrianglesVis::destroy_ispc()
{
  if (ispc)
  {
    ispc::TrianglesVis_destroy(ispc);
  }
}

bool
TrianglesVis::local_commit(MPI_Comm c)
{  
	return super::local_commit(c);
}
}
