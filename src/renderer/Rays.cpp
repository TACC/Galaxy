// ========================================================================== //
// Copyright (c) 2014-2020 The University of Texas at Austin.                 //
// All rights reserved.                                                       //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// A copy of the License is included with this software in the file LICENSE.  //
// If your copy does not contain the License, you may obtain a copy of the    //
// License at:                                                                //
//                                                                            //
//     https://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  //
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
//                                                                            //
// ========================================================================== //

#include "RayFlags.h"
#include "Rays.h"
// #include "Rays.ih"
#include "Rays_ispc.h"

#include <iostream>
#include <sstream>
#include <pthread.h>

#include "Application.h"
#include "Renderer.h"

using namespace std;

#define ROUND_UP_TO_MULTIPLE_OF_16(a) ((a + 15) & (~15))
#define ROUND_UP_TO_MULTIPLE_OF_64(a) ((a + 63) & (~63))
#define HDRSZ  ROUND_UP_TO_MULTIPLE_OF_64(sizeof(hdr))

namespace gxy
{

RayList::RayList(RendererDPtr renderer, RenderingSetDPtr rs, RenderingDPtr r, int nrays, RayListType type) 
	: RayList(renderer, rs, r, nrays, rs->GetCurrentFrame(), type) {}

static pthread_mutex_t raylist_lock = PTHREAD_MUTEX_INITIALIZER;
static int raylist_id = 0;

RayList::RayList(RendererDPtr renderer, RenderingSetDPtr rs, RenderingDPtr r, int nrays, int frame, RayListType type)
{
	theRenderer = renderer;
	theRenderingSet = rs;
	theRendering    = r;

	// Want arrays to be aligned on 64 bytes, so round number of rays up to multiple of 16
	// Need 64 bytes for header but leaves arrays aligned

	int nn = ROUND_UP_TO_MULTIPLE_OF_16(nrays);
	contents = smem::New(HDRSZ + nn * (20*sizeof(float) + 5*sizeof(int)));
	
	hdr *h  = (hdr *)contents->get();
	h->frame        		= frame;
	h->rendererKey			= renderer->getkey();
	h->renderingKey			= r->getkey();
	h->renderingSetKey	    = rs->getkey();
	h->size 				= nrays;
	h->aligned_size 		= nn;
	h->type 				= type;

	ispc = malloc(sizeof(::ispc::RayList_ispc));
	setup_ispc_pointers();

	pthread_mutex_lock(&raylist_lock);
	h->id = raylist_id++;
	pthread_mutex_unlock(&raylist_lock);
};

RayList::RayList(int n)
{
    theRenderer = NULL;
    theRenderingSet = NULL;
    theRendering = NULL;

    int nn = ROUND_UP_TO_MULTIPLE_OF_16(n);
    contents = smem::New(HDRSZ + nn * (20*sizeof(float) + 5*sizeof(int)));

	hdr *h  = (hdr *)contents->get();
	h->frame        		= -1;
	h->rendererKey			= -1;
	h->renderingKey			= -1;
	h->renderingSetKey	    = -1;
	h->size 				= n;
	h->aligned_size 		= nn;
	h->type 				= PRIMARY;

	ispc = malloc(sizeof(::ispc::RayList_ispc));
	setup_ispc_pointers();

	pthread_mutex_lock(&raylist_lock);
	h->id = raylist_id++;
	pthread_mutex_unlock(&raylist_lock);
}

RayList::RayList(SharedP c)
{
	contents = c;

	hdr *h  = (hdr *)contents->get();
	theRenderer = Renderer::GetByKey(h->rendererKey);
	theRenderingSet = RenderingSet::GetByKey(h->renderingSetKey);
	theRendering = Rendering::GetByKey(h->renderingKey);

	ispc = malloc(sizeof(::ispc::RayList_ispc));
	setup_ispc_pointers();
}

RayList::~RayList()
{
  free(ispc);
}

float RayList::get_ox(int i)              { return ((::ispc::RayList_ispc *)ispc)->ox[i]; }
float RayList::get_oy(int i)              { return ((::ispc::RayList_ispc *)ispc)->oy[i]; }
float RayList::get_oz(int i)              { return ((::ispc::RayList_ispc *)ispc)->oz[i]; }
float RayList::get_dx(int i)              { return ((::ispc::RayList_ispc *)ispc)->dx[i]; }
float RayList::get_dy(int i)              { return ((::ispc::RayList_ispc *)ispc)->dy[i]; }
float RayList::get_dz(int i)              { return ((::ispc::RayList_ispc *)ispc)->dz[i]; }
float RayList::get_nx(int i)              { return ((::ispc::RayList_ispc *)ispc)->nx[i]; }
float RayList::get_ny(int i)              { return ((::ispc::RayList_ispc *)ispc)->ny[i]; }
float RayList::get_nz(int i)              { return ((::ispc::RayList_ispc *)ispc)->nz[i]; }
float RayList::get_sample(int i)          { return ((::ispc::RayList_ispc *)ispc)->sample[i]; }
float RayList::get_r(int i)               { return ((::ispc::RayList_ispc *)ispc)->r[i]; }
float RayList::get_g(int i)               { return ((::ispc::RayList_ispc *)ispc)->g[i]; }
float RayList::get_b(int i)               { return ((::ispc::RayList_ispc *)ispc)->b[i]; }
float RayList::get_o(int i)               { return ((::ispc::RayList_ispc *)ispc)->o[i]; }
float RayList::get_sr(int i)              { return ((::ispc::RayList_ispc *)ispc)->sr[i]; }
float RayList::get_sg(int i)              { return ((::ispc::RayList_ispc *)ispc)->sg[i]; }
float RayList::get_sb(int i)              { return ((::ispc::RayList_ispc *)ispc)->sb[i]; }
float RayList::get_so(int i)              { return ((::ispc::RayList_ispc *)ispc)->so[i]; }
float RayList::get_t(int i)               { return ((::ispc::RayList_ispc *)ispc)->t[i]; }
float RayList::get_tMax(int i)            { return ((::ispc::RayList_ispc *)ispc)->tMax[i]; }
int   RayList::get_x(int i)               { return ((::ispc::RayList_ispc *)ispc)->x[i]; }
int   RayList::get_y(int i)               { return ((::ispc::RayList_ispc *)ispc)->y[i]; }
int   RayList::get_type(int i)            { return ((::ispc::RayList_ispc *)ispc)->type[i]; }
int   RayList::get_term(int i)            { return ((::ispc::RayList_ispc *)ispc)->term[i]; }
int   RayList::get_classification(int i)  { return ((::ispc::RayList_ispc *)ispc)->classification[i]; }

float* RayList::get_ox_base()             { return ((::ispc::RayList_ispc *)ispc)->ox; }
float* RayList::get_oy_base()             { return ((::ispc::RayList_ispc *)ispc)->oy; }
float* RayList::get_oz_base()             { return ((::ispc::RayList_ispc *)ispc)->oz; }
float* RayList::get_dx_base()             { return ((::ispc::RayList_ispc *)ispc)->dx; }
float* RayList::get_dy_base()             { return ((::ispc::RayList_ispc *)ispc)->dy; }
float* RayList::get_dz_base()             { return ((::ispc::RayList_ispc *)ispc)->dz; }
float* RayList::get_nx_base()             { return ((::ispc::RayList_ispc *)ispc)->nx; }
float* RayList::get_ny_base()             { return ((::ispc::RayList_ispc *)ispc)->ny; }
float* RayList::get_nz_base()             { return ((::ispc::RayList_ispc *)ispc)->nz; }
float* RayList::get_sample_base()         { return ((::ispc::RayList_ispc *)ispc)->sample; }
float* RayList::get_r_base()              { return ((::ispc::RayList_ispc *)ispc)->r; }
float* RayList::get_g_base()              { return ((::ispc::RayList_ispc *)ispc)->g; }
float* RayList::get_b_base()              { return ((::ispc::RayList_ispc *)ispc)->b; }
float* RayList::get_o_base()              { return ((::ispc::RayList_ispc *)ispc)->o; }
float* RayList::get_sr_base()             { return ((::ispc::RayList_ispc *)ispc)->sr; }
float* RayList::get_sg_base()             { return ((::ispc::RayList_ispc *)ispc)->sg; }
float* RayList::get_sb_base()             { return ((::ispc::RayList_ispc *)ispc)->sb; }
float* RayList::get_so_base()             { return ((::ispc::RayList_ispc *)ispc)->so; }
float* RayList::get_t_base()              { return ((::ispc::RayList_ispc *)ispc)->t; }
float* RayList::get_tMax_base()           { return ((::ispc::RayList_ispc *)ispc)->tMax; }
int*   RayList::get_x_base()              { return ((::ispc::RayList_ispc *)ispc)->x; }
int*   RayList::get_y_base()              { return ((::ispc::RayList_ispc *)ispc)->y; }
int*   RayList::get_type_base()           { return ((::ispc::RayList_ispc *)ispc)->type; }
int*   RayList::get_term_base()           { return ((::ispc::RayList_ispc *)ispc)->term; }
int*   RayList::get_classification_base() { return ((::ispc::RayList_ispc *)ispc)->classification; }

void RayList::set_ox(int i, float v)           { ((::ispc::RayList_ispc *)ispc)->ox[i] = v; }
void RayList::set_oy(int i, float v)           { ((::ispc::RayList_ispc *)ispc)->oy[i] = v; }
void RayList::set_oz(int i, float v)           { ((::ispc::RayList_ispc *)ispc)->oz[i] = v; }
void RayList::set_dx(int i, float v)           { ((::ispc::RayList_ispc *)ispc)->dx[i] = v; }
void RayList::set_dy(int i, float v)           { ((::ispc::RayList_ispc *)ispc)->dy[i] = v; }
void RayList::set_dz(int i, float v)           { ((::ispc::RayList_ispc *)ispc)->dz[i] = v; }
void RayList::set_nx(int i, float v)           { ((::ispc::RayList_ispc *)ispc)->nx[i] = v; }
void RayList::set_ny(int i, float v)           { ((::ispc::RayList_ispc *)ispc)->ny[i] = v; }
void RayList::set_nz(int i, float v)           { ((::ispc::RayList_ispc *)ispc)->nz[i] = v; }
void RayList::set_sample(int i, float v)       { ((::ispc::RayList_ispc *)ispc)->sample[i] = v; }
void RayList::set_r(int i, float v)            { ((::ispc::RayList_ispc *)ispc)->r[i] = v; }
void RayList::set_g(int i, float v)            { ((::ispc::RayList_ispc *)ispc)->g[i] = v; }
void RayList::set_b(int i, float v)            { ((::ispc::RayList_ispc *)ispc)->b[i] = v; }
void RayList::set_o(int i, float v)            { ((::ispc::RayList_ispc *)ispc)->o[i] = v; }
void RayList::set_sr(int i, float v)           { ((::ispc::RayList_ispc *)ispc)->sr[i] = v; }
void RayList::set_sg(int i, float v)           { ((::ispc::RayList_ispc *)ispc)->sg[i] = v; }
void RayList::set_sb(int i, float v)           { ((::ispc::RayList_ispc *)ispc)->sb[i] = v; }
void RayList::set_so(int i, float v)           { ((::ispc::RayList_ispc *)ispc)->so[i] = v; }
void RayList::set_t(int i, float v)            { ((::ispc::RayList_ispc *)ispc)->t[i] = v; }
void RayList::set_tMax(int i, float v)         { ((::ispc::RayList_ispc *)ispc)->tMax[i] = v; }
void RayList::set_x(int i, int v)              { ((::ispc::RayList_ispc *)ispc)->x[i] = v; }
void RayList::set_y(int i, int v)              { ((::ispc::RayList_ispc *)ispc)->y[i] = v; }
void RayList::set_type(int i, int v)           { ((::ispc::RayList_ispc *)ispc)->type[i] = v; }
void RayList::set_term(int i, int v)           { ((::ispc::RayList_ispc *)ispc)->term[i] = v; }
void RayList::set_classification(int i, int v) { ((::ispc::RayList_ispc *)ispc)->classification[i] = v; }

void *
RayList::get_base()
{
    return (void *)(contents->get() + HDRSZ);
}

void
RayList::setup_ispc_pointers()
{
	hdr *h = (hdr *)contents->get();
    int nn = h->aligned_size;

	((::ispc::RayList_ispc *)ispc)->ox             = (float *)get_base();
	((::ispc::RayList_ispc *)ispc)->oy             = ((::ispc::RayList_ispc *)ispc)->ox + nn;
	((::ispc::RayList_ispc *)ispc)->oz             = ((::ispc::RayList_ispc *)ispc)->oy + nn;
	((::ispc::RayList_ispc *)ispc)->dx             = ((::ispc::RayList_ispc *)ispc)->oz + nn;
	((::ispc::RayList_ispc *)ispc)->dy             = ((::ispc::RayList_ispc *)ispc)->dx + nn;
	((::ispc::RayList_ispc *)ispc)->dz             = ((::ispc::RayList_ispc *)ispc)->dy + nn;
	((::ispc::RayList_ispc *)ispc)->nx             = ((::ispc::RayList_ispc *)ispc)->dz + nn;
	((::ispc::RayList_ispc *)ispc)->ny             = ((::ispc::RayList_ispc *)ispc)->nx + nn;
	((::ispc::RayList_ispc *)ispc)->nz             = ((::ispc::RayList_ispc *)ispc)->ny + nn;
	((::ispc::RayList_ispc *)ispc)->sample         = ((::ispc::RayList_ispc *)ispc)->nz + nn;
	((::ispc::RayList_ispc *)ispc)->r              = ((::ispc::RayList_ispc *)ispc)->sample + nn;
	((::ispc::RayList_ispc *)ispc)->g              = ((::ispc::RayList_ispc *)ispc)->r + nn;
	((::ispc::RayList_ispc *)ispc)->b              = ((::ispc::RayList_ispc *)ispc)->g + nn;
	((::ispc::RayList_ispc *)ispc)->o              = ((::ispc::RayList_ispc *)ispc)->b + nn;
	((::ispc::RayList_ispc *)ispc)->sr             = ((::ispc::RayList_ispc *)ispc)->o + nn;
	((::ispc::RayList_ispc *)ispc)->sg             = ((::ispc::RayList_ispc *)ispc)->sr + nn;
	((::ispc::RayList_ispc *)ispc)->sb             = ((::ispc::RayList_ispc *)ispc)->sg + nn;
	((::ispc::RayList_ispc *)ispc)->so             = ((::ispc::RayList_ispc *)ispc)->sb + nn;
	((::ispc::RayList_ispc *)ispc)->t              = ((::ispc::RayList_ispc *)ispc)->so + nn;
	((::ispc::RayList_ispc *)ispc)->tMax           = ((::ispc::RayList_ispc *)ispc)->t + nn;
	((::ispc::RayList_ispc *)ispc)->x              = (int *)(((::ispc::RayList_ispc *)ispc)->tMax + nn);
	((::ispc::RayList_ispc *)ispc)->y              = ((::ispc::RayList_ispc *)ispc)->x + nn;
	((::ispc::RayList_ispc *)ispc)->type           = ((::ispc::RayList_ispc *)ispc)->y + nn;
	((::ispc::RayList_ispc *)ispc)->term           = ((::ispc::RayList_ispc *)ispc)->type + nn;
	((::ispc::RayList_ispc *)ispc)->classification = ((::ispc::RayList_ispc *)ispc)->term + nn;
}

void
RayList::Split(vector<RayList*>& subsets)
{
  int rpp = GetTheRenderer()->GetMaxRayListSize();
  for (int i = 0; i < GetRayCount(); i += rpp)
  {
    int this_rpp = ((i + rpp) > GetRayCount()) ? GetRayCount() - i : rpp;
 
    RayList *part = new RayList(GetTheRenderer(), GetTheRenderingSet(), GetTheRendering(), this_rpp, GetFrame(), GetType());

    memcpy(part->get_ox_base(),     get_ox_base()     + i, this_rpp*sizeof(float));
    memcpy(part->get_oy_base(),     get_oy_base()     + i, this_rpp*sizeof(float));
    memcpy(part->get_oz_base(),     get_oz_base()     + i, this_rpp*sizeof(float));
    memcpy(part->get_dx_base(),     get_dx_base()     + i, this_rpp*sizeof(float));
    memcpy(part->get_dy_base(),     get_dy_base()     + i, this_rpp*sizeof(float));
    memcpy(part->get_dz_base(),     get_dz_base()     + i, this_rpp*sizeof(float));
    memcpy(part->get_nx_base(),     get_nx_base()     + i, this_rpp*sizeof(float));
    memcpy(part->get_ny_base(),     get_ny_base()     + i, this_rpp*sizeof(float));
    memcpy(part->get_nz_base(),     get_nz_base()     + i, this_rpp*sizeof(float));
    memcpy(part->get_sample_base(), get_sample_base() + i, this_rpp*sizeof(float));
    memcpy(part->get_r_base(),      get_r_base()      + i, this_rpp*sizeof(float));
    memcpy(part->get_g_base(),      get_g_base()      + i, this_rpp*sizeof(float));
    memcpy(part->get_b_base(),      get_b_base()      + i, this_rpp*sizeof(float));
    memcpy(part->get_o_base(),      get_o_base()      + i, this_rpp*sizeof(float));
    memcpy(part->get_sr_base(),     get_sr_base()     + i, this_rpp*sizeof(float));
    memcpy(part->get_sg_base(),     get_sg_base()     + i, this_rpp*sizeof(float));
    memcpy(part->get_sb_base(),     get_sb_base()     + i, this_rpp*sizeof(float));
    memcpy(part->get_so_base(),     get_so_base()     + i, this_rpp*sizeof(float));
    memcpy(part->get_t_base(),      get_t_base()      + i, this_rpp*sizeof(float));
    memcpy(part->get_tMax_base(),   get_tMax_base()   + i, this_rpp*sizeof(float));
    memcpy(part->get_x_base(),      get_x_base()      + i, this_rpp*sizeof(int));
    memcpy(part->get_y_base(),      get_y_base()      + i, this_rpp*sizeof(int));
    memcpy(part->get_type_base(),   get_type_base()   + i, this_rpp*sizeof(int));
    memcpy(part->get_term_base(),   get_term_base()   + i, this_rpp*sizeof(int));

    subsets.push_back(part);
  }
} 

void
RayList::Truncate(int n)
{
	hdr *old_h  = (hdr *)contents->get();

	int new_aligned_size = ROUND_UP_TO_MULTIPLE_OF_16(n);

	if (new_aligned_size < old_h->aligned_size)
	{
		SharedP old_contents = contents;
		contents = smem::New(HDRSZ + new_aligned_size * (20*sizeof(float) + 5*sizeof(int)));

		hdr *new_h = (hdr *)contents->get();

		new_h->renderingSetKey  = old_h->renderingSetKey;
		new_h->renderingKey     = old_h->renderingKey;
		new_h->frame         		= old_h->frame;
		new_h->size         		= n;
		new_h->aligned_size 		= new_aligned_size;

		unsigned char *src = old_contents->get() + HDRSZ;
		unsigned char *dst = contents->get() + HDRSZ;

		for (int i = 0; i < 20; i++)
		{
			memcpy(dst, src, n*sizeof(float));
			src += old_h->aligned_size*sizeof(float);
			dst += new_h->aligned_size*sizeof(float);
		}

		for (int i = 0; i < 5; i++)
		{
			memcpy(dst, src, n*sizeof(int));
			src += old_h->aligned_size*sizeof(int);
			dst += new_h->aligned_size*sizeof(int);
		}

		setup_ispc_pointers();
	}
	else
		old_h->size = n;
}

void
RayList::print(int which)
{
#if 0
#if 1
	std::stringstream s;
#else
	std::ofstream s("rays.out");
#endif
#else
	std::ostream &s = std::cerr;
#endif

	s << "px,py,ox,oy,oz,dx,dy,dz,nx,ny,nz,cx,cy,cz,r,g,b,classification,t,tMax,primary,shadow,ao,empty,surface,opaque,boundary,timeout" << endl;
	if (which == -1)
		for (int i = 0; i < GetRayCount(); i++)
		{
			s << get_x(i) << "," << get_y(i) << ",";
			s << get_ox(i) << "," << get_oy(i) << "," << get_oz(i) << ",";
			s << get_dx(i) << "," << get_dy(i) << "," << get_dz(i) << ",";
			s << get_nx(i) << "," << get_ny(i) << "," << get_nz(i) << ",";
			s << (get_ox(i) + get_t(i)*get_dx(i)) << "," << (get_oy(i) + get_t(i)*get_dy(i)) << "," << (get_oz(i) + get_t(i)*get_dz(i)) << ",";
			s << get_r(i) << "," << get_g(i) << "," << get_b(i) << ",";
			s << get_classification(i) << ",";
			s << get_t(i) << "," << get_tMax(i) << ",";
			s << (get_type(i) == RAY_PRIMARY ? 1 : 0) << ",";
			s << (get_type(i) == RAY_SHADOW  ? 1 : 0) << ",";
			s << (get_type(i) == RAY_AO      ? 1 : 0) << ",";
			s << (get_type(i) == RAY_EMPTY   ? 1 : 0) << ",";
			s << (get_term(i) & RAY_SURFACE  ? 1 : 0) << ",";
			s << (get_term(i) & RAY_OPAQUE   ? 1 : 0) << ",";
			s << (get_term(i) & RAY_BOUNDARY ? 1 : 0) << ",";
			s << (get_term(i) & RAY_TIMEOUT  ? 1 : 0) << endl;
		}
	else
		{
			s << get_x(which) << "," << get_y(which) << ",";
			s << get_ox(which) << "," << get_oy(which) << "," << get_oz(which) << ",";
			s << get_dx(which) << "," << get_dy(which) << "," << get_dz(which) << ",";
			s << (get_ox(which) + get_t(which)*get_dx(which)) << "," << (get_oy(which) + get_t(which)*get_dy(which)) << "," << (get_oz(which) + get_t(which)*get_dz(which)) << ",";
			s << get_r(which) << "," << get_g(which) << "," << get_b(which) << ",";
			s << get_classification(which) << ",";
			s << get_t(which) << "," << get_tMax(which) << ",";
			s << (get_type(which) == RAY_PRIMARY ? 1 : 0) << ",";
			s << (get_type(which) == RAY_SHADOW  ? 1 : 0) << ",";
			s << (get_type(which) == RAY_AO      ? 1 : 0) << ",";
			s << (get_type(which) == RAY_EMPTY   ? 1 : 0) << ",";
			s << (get_term(which) & RAY_SURFACE  ? 1 : 0) << ",";
			s << (get_term(which) & RAY_OPAQUE   ? 1 : 0) << ",";
			s << (get_term(which) & RAY_BOUNDARY ? 1 : 0) << ",";
			s << (get_term(which) & RAY_TIMEOUT  ? 1 : 0) << endl;
		}
#if 0
#if 1
	GetTheApplication()->Print(s.str());
#else
	s.close();
#endif
#endif
}

} // namespace gxy
