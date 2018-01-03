#include "Vis.ih"

export void *uniform Vis_allocate()
{
    Vis_ispc *uniform v = uniform new uniform Vis_ispc;
    return (void *)v;
}

export void *uniform Vis_initialize(void *uniform _self)
{
    Vis_ispc *uniform self = (uniform Vis_ispc *)_self;
    self->data = NULL;
}

export void *uniform Vis_destroy(void *uniform _self)
{
    delete _self;
}

export void Vis_set_data(void *uniform _self, void *uniform d)
{
    Vis_ispc *uniform self = (uniform Vis_ispc *)_self;
    self->data = d;
}

 