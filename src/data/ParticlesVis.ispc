#include "ParticlesVis.ih"

export void *uniform ParticlesVis_allocate()
{
	ParticlesVis_ispc *uniform v = uniform new uniform ParticlesVis_ispc;
	return (void *)v;
}

export void ParticlesVis_initialize(void *uniform _self)
{
	ParticlesVis_ispc *uniform self = (uniform ParticlesVis_ispc *)_self;
}

export void ParticlesVis_destroy(void *uniform _self)
{
}
