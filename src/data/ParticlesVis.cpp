#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream>
#include "Application.h"
#include "ParticlesVis.h"
#include "Datasets.h"

#include "ParticlesVis_ispc.h"

KEYED_OBJECT_TYPE(ParticlesVis)

void
ParticlesVis::Register()
{
  RegisterClass();
}

ParticlesVis::~ParticlesVis()
{
	ParticlesVis::destroy_ispc();
}

void
ParticlesVis::initialize()
{
  Vis::initialize();
}

void
ParticlesVis::initialize_ispc()
{
  super::initialize_ispc();
  ispc::ParticlesVis_initialize(ispc);
} 
    
void
ParticlesVis::allocate_ispc()
{
  ispc = ispc::ParticlesVis_allocate();
}

int 
ParticlesVis::serialSize()
{
  return super::serialSize();
}

unsigned char *
ParticlesVis::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);
  return ptr;
}

unsigned char *
ParticlesVis::deserialize(unsigned char *ptr)
{
  ptr = super::deserialize(ptr);
  return ptr;
}

void 
ParticlesVis::LoadFromJSON(Value& v)
{
  super::LoadFromJSON(v);
}

void
ParticlesVis::SaveToJSON(Value& v, Document&  doc)
{
  Vis::SaveToJSON(v, doc);
}

void
ParticlesVis::destroy_ispc()
{
  if (ispc)
  {
    ispc::ParticlesVis_destroy(ispc);
  }
}

bool
ParticlesVis::local_commit(MPI_Comm c)
{  
	return super::local_commit(c);
}
