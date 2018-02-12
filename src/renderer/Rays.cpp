#include <iostream>
#include <sstream>
#include "Application.h"
#include "Rays.h"
#include "RayFlags.h"
#include "Rays.ih"
#include "Rays_ispc.h"

#define ROUND_UP_TO_MULTIPLE_OF_16(a) ((a + 15) & (~15))
#define ROUND_UP_TO_MULTIPLE_OF_64(a) ((a + 63) & (~63))

#define HDRSZ  ROUND_UP_TO_MULTIPLE_OF_64(sizeof(hdr))

RayList::RayList(RenderingP r, int nrays) : RayList(nrays)
{
	hdr *h  = (hdr *)contents->get();
	h->frame        = r->GetFrame();
	h->key  				= r->getkey();
};

RayList::RayList(int nrays) 
{
	// Want arrays to be aligned on 64 bytes, so round number of rays up to multiple of 16

	int nn = ROUND_UP_TO_MULTIPLE_OF_16(nrays);

	// Need 64 bytes for header (which is prob. just the key) but leaves arrays aligned

	contents = smem::New(HDRSZ + nn * (20*sizeof(float) + 4*sizeof(int)));
	hdr *h  = (hdr *)contents->get();

	h->key  				= -1;
	h->size 				= nrays;
	h->aligned_size = nn;

	ispc = malloc(sizeof(ispc::RayList_ispc));
	setup_ispc_pointers();
};

RayList::RayList(SharedP c)
{
	contents = c;

	ispc = malloc(sizeof(ispc::RayList_ispc));
	setup_ispc_pointers();
}

RayList::~RayList()
{
  free(ispc);
}

int
RayList::GetFrame()
{
	return (((hdr *)contents->get()))->frame;
}

float RayList::get_ox(int i)     { return ((ispc::RayList_ispc *)ispc)->ox[i]; }
float RayList::get_oy(int i)     { return ((ispc::RayList_ispc *)ispc)->oy[i]; }
float RayList::get_oz(int i)     { return ((ispc::RayList_ispc *)ispc)->oz[i]; }
float RayList::get_dx(int i)     { return ((ispc::RayList_ispc *)ispc)->dx[i]; }
float RayList::get_dy(int i)     { return ((ispc::RayList_ispc *)ispc)->dy[i]; }
float RayList::get_dz(int i)     { return ((ispc::RayList_ispc *)ispc)->dz[i]; }
float RayList::get_nx(int i)     { return ((ispc::RayList_ispc *)ispc)->nx[i]; }
float RayList::get_ny(int i)     { return ((ispc::RayList_ispc *)ispc)->ny[i]; }
float RayList::get_nz(int i)     { return ((ispc::RayList_ispc *)ispc)->nz[i]; }
float RayList::get_sample(int i) { return ((ispc::RayList_ispc *)ispc)->sample[i]; }
float RayList::get_r(int i)      { return ((ispc::RayList_ispc *)ispc)->r[i]; }
float RayList::get_g(int i)      { return ((ispc::RayList_ispc *)ispc)->g[i]; }
float RayList::get_b(int i)      { return ((ispc::RayList_ispc *)ispc)->b[i]; }
float RayList::get_o(int i)      { return ((ispc::RayList_ispc *)ispc)->o[i]; }
float RayList::get_sr(int i)     { return ((ispc::RayList_ispc *)ispc)->sr[i]; }
float RayList::get_sg(int i)     { return ((ispc::RayList_ispc *)ispc)->sg[i]; }
float RayList::get_sb(int i)     { return ((ispc::RayList_ispc *)ispc)->sb[i]; }
float RayList::get_so(int i)     { return ((ispc::RayList_ispc *)ispc)->so[i]; }
float RayList::get_t(int i)      { return ((ispc::RayList_ispc *)ispc)->t[i]; }
float RayList::get_tMax(int i)   { return ((ispc::RayList_ispc *)ispc)->tMax[i]; }
int   RayList::get_x(int i)      { return ((ispc::RayList_ispc *)ispc)->x[i]; }
int   RayList::get_y(int i)      { return ((ispc::RayList_ispc *)ispc)->y[i]; }
int   RayList::get_type(int i)   { return ((ispc::RayList_ispc *)ispc)->type[i]; }
int   RayList::get_term(int i)   { return ((ispc::RayList_ispc *)ispc)->term[i]; }

void RayList::set_ox(int i, float v)     { ((ispc::RayList_ispc *)ispc)->ox[i] = v; }
void RayList::set_oy(int i, float v)     { ((ispc::RayList_ispc *)ispc)->oy[i] = v; }
void RayList::set_oz(int i, float v)     { ((ispc::RayList_ispc *)ispc)->oz[i] = v; }
void RayList::set_dx(int i, float v)     { ((ispc::RayList_ispc *)ispc)->dx[i] = v; }
void RayList::set_dy(int i, float v)     { ((ispc::RayList_ispc *)ispc)->dy[i] = v; }
void RayList::set_dz(int i, float v)     { ((ispc::RayList_ispc *)ispc)->dz[i] = v; }
void RayList::set_nx(int i, float v)     { ((ispc::RayList_ispc *)ispc)->nx[i] = v; }
void RayList::set_ny(int i, float v)     { ((ispc::RayList_ispc *)ispc)->ny[i] = v; }
void RayList::set_nz(int i, float v)     { ((ispc::RayList_ispc *)ispc)->nz[i] = v; }
void RayList::set_sample(int i, float v) { ((ispc::RayList_ispc *)ispc)->sample[i] = v; }
void RayList::set_r(int i, float v)      { ((ispc::RayList_ispc *)ispc)->r[i] = v; }
void RayList::set_g(int i, float v)      { ((ispc::RayList_ispc *)ispc)->g[i] = v; }
void RayList::set_b(int i, float v)      { ((ispc::RayList_ispc *)ispc)->b[i] = v; }
void RayList::set_o(int i, float v)      { ((ispc::RayList_ispc *)ispc)->o[i] = v; }
void RayList::set_sr(int i, float v)     { ((ispc::RayList_ispc *)ispc)->sr[i] = v; }
void RayList::set_sg(int i, float v)     { ((ispc::RayList_ispc *)ispc)->sg[i] = v; }
void RayList::set_sb(int i, float v)     { ((ispc::RayList_ispc *)ispc)->sb[i] = v; }
void RayList::set_so(int i, float v)     { ((ispc::RayList_ispc *)ispc)->so[i] = v; }
void RayList::set_t(int i, float v)      { ((ispc::RayList_ispc *)ispc)->t[i] = v; }
void RayList::set_tMax(int i, float v)   { ((ispc::RayList_ispc *)ispc)->tMax[i] = v; }
void RayList::set_x(int i, int v)        { ((ispc::RayList_ispc *)ispc)->x[i] = v; }
void RayList::set_y(int i, int v)        { ((ispc::RayList_ispc *)ispc)->y[i] = v; }
void RayList::set_type(int i, int v)     { ((ispc::RayList_ispc *)ispc)->type[i] = v; }
void RayList::set_term(int i, int v)     { ((ispc::RayList_ispc *)ispc)->term[i] = v; }

void
RayList::setup_ispc_pointers()
{
	hdr *h = (hdr *)contents->get();
  int nn = h->aligned_size;

	((ispc::RayList_ispc *)ispc)->ox     = (float *)(contents->get() + HDRSZ); // Skipping first 64 bytes for header
	((ispc::RayList_ispc *)ispc)->oy     = ((ispc::RayList_ispc *)ispc)->ox + nn;
	((ispc::RayList_ispc *)ispc)->oz     = ((ispc::RayList_ispc *)ispc)->oy + nn;
	((ispc::RayList_ispc *)ispc)->dx     = ((ispc::RayList_ispc *)ispc)->oz + nn;
	((ispc::RayList_ispc *)ispc)->dy     = ((ispc::RayList_ispc *)ispc)->dx + nn;
	((ispc::RayList_ispc *)ispc)->dz     = ((ispc::RayList_ispc *)ispc)->dy + nn;
	((ispc::RayList_ispc *)ispc)->nx     = ((ispc::RayList_ispc *)ispc)->dz + nn;
	((ispc::RayList_ispc *)ispc)->ny     = ((ispc::RayList_ispc *)ispc)->nx + nn;
	((ispc::RayList_ispc *)ispc)->nz     = ((ispc::RayList_ispc *)ispc)->ny + nn;
	((ispc::RayList_ispc *)ispc)->sample = ((ispc::RayList_ispc *)ispc)->nz + nn;
	((ispc::RayList_ispc *)ispc)->r      = ((ispc::RayList_ispc *)ispc)->sample + nn;
	((ispc::RayList_ispc *)ispc)->g      = ((ispc::RayList_ispc *)ispc)->r + nn;
	((ispc::RayList_ispc *)ispc)->b      = ((ispc::RayList_ispc *)ispc)->g + nn;
	((ispc::RayList_ispc *)ispc)->o      = ((ispc::RayList_ispc *)ispc)->b + nn;
	((ispc::RayList_ispc *)ispc)->sr     = ((ispc::RayList_ispc *)ispc)->o + nn;
	((ispc::RayList_ispc *)ispc)->sg     = ((ispc::RayList_ispc *)ispc)->sr + nn;
	((ispc::RayList_ispc *)ispc)->sb     = ((ispc::RayList_ispc *)ispc)->sg + nn;
	((ispc::RayList_ispc *)ispc)->so     = ((ispc::RayList_ispc *)ispc)->sb + nn;
	((ispc::RayList_ispc *)ispc)->t      = ((ispc::RayList_ispc *)ispc)->so + nn;
	((ispc::RayList_ispc *)ispc)->tMax   = ((ispc::RayList_ispc *)ispc)->t + nn;
	((ispc::RayList_ispc *)ispc)->x      = (int *)(((ispc::RayList_ispc *)ispc)->tMax + nn);
	((ispc::RayList_ispc *)ispc)->y      = ((ispc::RayList_ispc *)ispc)->x + nn;
	((ispc::RayList_ispc *)ispc)->type   = ((ispc::RayList_ispc *)ispc)->y + nn;
	((ispc::RayList_ispc *)ispc)->term   = ((ispc::RayList_ispc *)ispc)->type + nn;
}

void
RayList::Truncate(int n)
{
	hdr *old_h  = (hdr *)contents->get();

	int new_aligned_size = ROUND_UP_TO_MULTIPLE_OF_16(n);

	if (new_aligned_size < old_h->aligned_size)
	{
		SharedP old_contents = contents;
		contents = smem::New(HDRSZ + new_aligned_size * (20*sizeof(float) + 4*sizeof(int)));

		hdr *new_h = (hdr *)contents->get();

		new_h->key          = old_h->key;
		new_h->frame        = old_h->frame;
		new_h->size         = n;
		new_h->aligned_size = new_aligned_size;

		unsigned char *src = old_contents->get() + HDRSZ;
		unsigned char *dst = contents->get() + HDRSZ;

		for (int i = 0; i < 19; i++)
		{
			memcpy(dst, src, n*sizeof(float));
			src += old_h->aligned_size*sizeof(float);
			dst += new_h->aligned_size*sizeof(float);
		}

		for (int i = 0; i < 4; i++)
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

RenderingP
RayList::GetTheRendering()
{
	return Rendering::GetByKey(GetTheRenderingKey());
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

	s << "px,py,ox,oy,oz,dx,dy,dz,cx,cy,cz,r,g,b,t,tMax,primary,shadow,ao,empty,surface,opaque,boundary,timeout\n";
	if (which == -1)
		for (int i = 0; i < GetRayCount(); i++)
		{
			s << get_x(i) << "," << get_y(i) << ",";
			s << get_ox(i) << "," << get_oy(i) << "," << get_oz(i) << ",";
			s << get_dx(i) << "," << get_dy(i) << "," << get_dz(i) << ",";
			s << (get_ox(i) + get_t(i)*get_dx(i)) << "," << (get_oy(i) + get_t(i)*get_dy(i)) << "," << (get_oz(i) + get_t(i)*get_dz(i)) << ",";
			s << get_r(i) << "," << get_g(i) << "," << get_b(i) << ",";
			s << get_t(i) << "," << get_tMax(i) << ",";
			s << (get_type(i) == RAY_PRIMARY ? 1 : 0) << ",";
			s << (get_type(i) == RAY_SHADOW  ? 1 : 0) << ",";
			s << (get_type(i) == RAY_AO      ? 1 : 0) << ",";
			s << (get_type(i) == RAY_EMPTY   ? 1 : 0) << ",";
			s << (get_term(i) & RAY_SURFACE  ? 1 : 0) << ",";
			s << (get_term(i) & RAY_OPAQUE   ? 1 : 0) << ",";
			s << (get_term(i) & RAY_BOUNDARY ? 1 : 0) << ",";
			s << (get_term(i) & RAY_TIMEOUT  ? 1 : 0) << "\n";
		}
	else
		{
			s << get_x(which) << "," << get_y(which) << ",";
			s << get_ox(which) << "," << get_oy(which) << "," << get_oz(which) << ",";
			s << get_dx(which) << "," << get_dy(which) << "," << get_dz(which) << ",";
			s << (get_ox(which) + get_t(which)*get_dx(which)) << "," << (get_oy(which) + get_t(which)*get_dy(which)) << "," << (get_oz(which) + get_t(which)*get_dz(which)) << ",";
			s << get_r(which) << "," << get_g(which) << "," << get_b(which) << ",";
			s << get_t(which) << "," << get_tMax(which) << ",";
			s << (get_type(which) == RAY_PRIMARY ? 1 : 0) << ",";
			s << (get_type(which) == RAY_SHADOW  ? 1 : 0) << ",";
			s << (get_type(which) == RAY_AO      ? 1 : 0) << ",";
			s << (get_type(which) == RAY_EMPTY   ? 1 : 0) << ",";
			s << (get_term(which) & RAY_SURFACE  ? 1 : 0) << ",";
			s << (get_term(which) & RAY_OPAQUE   ? 1 : 0) << ",";
			s << (get_term(which) & RAY_BOUNDARY ? 1 : 0) << ",";
			s << (get_term(which) & RAY_TIMEOUT  ? 1 : 0) << "\n";
		}
#if 0
#if 1
	GetTheApplication()->Print(s.str());
#else
	s.close();
#endif
#endif
}
