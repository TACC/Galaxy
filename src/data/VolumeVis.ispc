#include "VolumeVis.ih"

export void *uniform VolumeVis_allocate()
{
	VolumeVis_ispc *uniform v = uniform new uniform VolumeVis_ispc;
	return (void *)v;
}

export void VolumeVis_initialize(void *uniform _self)
{
	VolumeVis_ispc *uniform self = (uniform VolumeVis_ispc *)_self;
	self->slices = NULL;
	self->nSlices = 0;
	self->isovalues = NULL;
	self->nIsovalues = 0;
}

export void VolumeVis_destroy(void *uniform _self)
{
	VolumeVis_ispc *uniform self = (uniform VolumeVis_ispc *)_self;

	if (self->slices)
	{
		delete[] self->slices;
		self->slices = NULL;
		self->nSlices = 0;
	}
    
	if (self->slices)
	{
		delete[] self->slices;
		self->slices = NULL;
		self->nSlices = 0;
	}
}

export void VolumeVis_SetSlices(void *uniform _self, uniform int n, uniform float *uniform sl)
{
	VolumeVis_ispc *uniform self = (uniform VolumeVis_ispc *)_self;
	
	if (n != self->nSlices)
	{
		delete[] self->slices;
		self->slices = uniform new vec4f[n];
		self->nSlices = n;
	}

	for (uniform int i = 0; i < n; i++)
    self->slices[i] = make_vec4f(sl[4*i + 0], sl[4*i + 1], sl[4*i + 2], sl[4*i + 3]);
}

export void VolumeVis_SetIsovalues(void *uniform _self, uniform int n, uniform float *uniform iso)
{
  VolumeVis_ispc *uniform self = (uniform VolumeVis_ispc *)_self;

  if (n != self->nIsovalues)
	{
		delete[] self->isovalues;
		self->isovalues = uniform new float[n];
		self->nIsovalues = n;
	}

	for (uniform int i = 0; i < n; i++)
    self->isovalues[i] = iso[i];
}

export void VolumeVis_SetVolumeRenderFlag(void *uniform _self, uniform bool b)
{
  VolumeVis_ispc *uniform self = (uniform VolumeVis_ispc *)_self;
	self->volume_render = b;
}
