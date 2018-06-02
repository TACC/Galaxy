#include "TrianglesVis.ih"

export void *uniform TrianglesVis_allocate()
{
	TrianglesVis_ispc *uniform v = uniform new uniform TrianglesVis_ispc;
	return (void *)v;
}

export void TrianglesVis_initialize(void *uniform _self)
{
	TrianglesVis_ispc *uniform self = (uniform TrianglesVis_ispc *)_self;
}

export void TrianglesVis_destroy(void *uniform _self)
{
}
