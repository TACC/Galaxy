#include "ISPCObject.ih"

export void *uniform ISPCObject_allocate()
{
  ISPCObject_ispc *uniform o = uniform new uniform ISPCObject_ispc;
  return (void *)o;
}

export void* uniform ISPCObject_initialize(void *uniform _self)
{
  ISPCObject_ispc *uniform self = (uniform ISPCObject_ispc *)_self;
  self->foo = 0;
 }

export void ISPCObject_destroy(void *uniform self)
{
  delete self;
}

	
