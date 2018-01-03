#include "MappedVis.ih"

export void *uniform MappedVis_allocate()
{
    MappedVis_ispc *uniform v = uniform new uniform MappedVis_ispc;
    return (void *)v;
}

export void MappedVis_initialize(void *uniform _self)
{
    MappedVis_ispc *uniform self = (uniform MappedVis_ispc *)_self;
    self->transferFunction = NULL;
}

export void MappedVis_set_transferFunction(void *uniform _self, void *uniform d)
{
    MappedVis_ispc *uniform self = (uniform MappedVis_ispc *)_self;
    self->transferFunction = d;
}                                                                                                                            


