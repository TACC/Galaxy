#include "Lighting.ih"

export void *uniform Lighting_allocate()
{
  Lighting_ispc *uniform t = uniform new uniform Lighting_ispc;
  return (void *)t;
}

export void Lighting_initialize(void *uniform _self)
{
  Lighting_ispc *uniform self = (uniform Lighting_ispc *)_self;

  self->n_ao_rays     = 0;
  self->do_shadows    = 0;
  self->ao_radius     = 1.0;
  self->Ka            = 0.5;
  self->Kd            = 0.5;
  self->lights        = uniform new vec3f[1];
  self->nLights       = 1;
  self->lights[0]     = make_vec3f(1, 1, 1);
  self->epsilon       = 0.001;
}

export void Lighting_destroy(void *uniform _self)
{
  Lighting_ispc *uniform self = (uniform Lighting_ispc *)_self;
  delete[] self->lights;
  delete _self;
}

export void Lighting_SetLights(void *uniform _self, uniform int n, uniform float *uniform l)
{
  Lighting_ispc *uniform self = (uniform Lighting_ispc *)_self;
	if (n != self->nLights)
	{
		delete[] self->lights;
		self->lights = uniform new vec3f[n];
		self->nLights = n;
	}

	for (uniform int i = 0; i < n; i++)
		self->lights[i] = make_vec3f(l[3*i + 0], l[3*i + 1], l[3*i + 2]);
}

export void Lighting_GetLights(void *uniform _self, uniform int *uniform n, uniform float *uniform *uniform l)
{
  Lighting_ispc *uniform self = (uniform Lighting_ispc *)_self;
	*n = self->nLights;
	*l = (uniform float *uniform)self->lights;
}

export void Lighting_SetK(void *uniform _self, uniform float ka, uniform float kd)
{
  Lighting_ispc *uniform self = (uniform Lighting_ispc *)_self;
	self->Ka = ka;
	self->Kd = kd;
}

export void Lighting_GetK(void *uniform _self, uniform float *uniform ka, uniform float *uniform kd)
{
  Lighting_ispc *uniform self = (uniform Lighting_ispc *)_self;
  *ka = self->Ka;
  *kd = self->Kd;
}
	
export void Lighting_SetAO(void *uniform _self, uniform int n, uniform float r)
{
  Lighting_ispc *uniform self = (uniform Lighting_ispc *)_self;
	self->n_ao_rays = n;
	self->ao_radius = r;
}
	
export void Lighting_GetAO(void *uniform _self, uniform int *uniform n, uniform float *uniform r)
{
  Lighting_ispc *uniform self = (uniform Lighting_ispc *)_self;
	*n = self->n_ao_rays;
	*r = self->ao_radius;
}
	
export void Lighting_SetShadowFlag(void *uniform _self, uniform int b)
{
  Lighting_ispc *uniform self = (uniform Lighting_ispc *)_self;
	self->do_shadows = b;
}
	
export void Lighting_GetShadowFlag(void *uniform _self, uniform int *uniform b)
{
  Lighting_ispc *uniform self = (uniform Lighting_ispc *)_self;
	*b = self->do_shadows;
}
	
export void Lighting_SetEpsilon(void *uniform _self, uniform float e)
{
  Lighting_ispc *uniform self = (uniform Lighting_ispc *)_self;
	self->epsilon = e;
}
	
export void Lighting_GetEpsilon(void *uniform _self, uniform float *uniform e)
{
  Lighting_ispc *uniform self = (uniform Lighting_ispc *)_self;
	*e = self->epsilon;
}
