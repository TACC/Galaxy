// ========================================================================== //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 //
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

#pragma once

#include <memory>
#include <vector>
#include "Rendering.h"
#include "smem.h"

namespace gxy
{

KEYED_OBJECT_POINTER(Rendering)
KEYED_OBJECT_POINTER(RenderingSet)

class RayList 
{
public:
	enum RayListType { PRIMARY, SECONDARY };

private:
	struct hdr
	{
		Key rendererKey;
		Key renderingKey;
		Key renderingSetKey;
		int size;
		int frame;
		int aligned_size;
		int id;
	  RayListType type;
	};

public:
	~RayList();

	static void CopyRay(RayList *srcRayList, int srcRayIndex, RayList *dstRayList, int dstRayIndex)
	{
    dstRayList->set_ox(dstRayIndex, srcRayList->get_ox(srcRayIndex));
    dstRayList->set_oy(dstRayIndex, srcRayList->get_oy(srcRayIndex));
    dstRayList->set_oz(dstRayIndex, srcRayList->get_oz(srcRayIndex));
    dstRayList->set_dx(dstRayIndex, srcRayList->get_dx(srcRayIndex));
    dstRayList->set_dy(dstRayIndex, srcRayList->get_dy(srcRayIndex));
    dstRayList->set_dz(dstRayIndex, srcRayList->get_dz(srcRayIndex));
    dstRayList->set_nx(dstRayIndex, srcRayList->get_nx(srcRayIndex));
    dstRayList->set_ny(dstRayIndex, srcRayList->get_ny(srcRayIndex));
    dstRayList->set_nz(dstRayIndex, srcRayList->get_nz(srcRayIndex));
    dstRayList->set_sample(dstRayIndex, srcRayList->get_sample(srcRayIndex));
    dstRayList->set_r(dstRayIndex, srcRayList->get_r(srcRayIndex));
    dstRayList->set_g(dstRayIndex, srcRayList->get_g(srcRayIndex));
    dstRayList->set_b(dstRayIndex, srcRayList->get_b(srcRayIndex));
    dstRayList->set_o(dstRayIndex, srcRayList->get_o(srcRayIndex));	
    dstRayList->set_sr(dstRayIndex, srcRayList->get_sr(srcRayIndex));
    dstRayList->set_sg(dstRayIndex, srcRayList->get_sg(srcRayIndex));
    dstRayList->set_sb(dstRayIndex, srcRayList->get_sb(srcRayIndex));
    dstRayList->set_so(dstRayIndex, srcRayList->get_so(srcRayIndex));
    dstRayList->set_t(dstRayIndex, srcRayList->get_t(srcRayIndex));
    dstRayList->set_tMax(dstRayIndex, srcRayList->get_tMax(srcRayIndex));
    dstRayList->set_x(dstRayIndex, srcRayList->get_x(srcRayIndex));
    dstRayList->set_y(dstRayIndex, srcRayList->get_y(srcRayIndex));
    dstRayList->set_type(dstRayIndex, srcRayList->get_type(srcRayIndex));
    dstRayList->set_term(dstRayIndex, srcRayList->get_term(srcRayIndex));
	}

	RayList(RendererP renderer, RenderingSetP rs, RenderingP r, int nrays, int frame, RayListType type);
	RayList(RendererP renderer, RenderingSetP rs, RenderingP r, int nrays, RayListType type);
	RayList(SharedP contents);

	RayListType  GetType() { return ((struct hdr *)contents->get())->type; }
	void SetType(RayListType t) { ((struct hdr *)contents->get())->type = t; };

	int GetFrame() { return ((struct hdr *)contents->get())->frame; }
	int GetRayCount() { return ((struct hdr *)contents->get())->size; }
	int GetId() { return ((struct hdr *)contents->get())->id; }
	SharedP get_ptr() { return contents; };

	void *get_header_address() { return ((void *)(contents->get())); }

	void Truncate(int n);
	void Split(std::vector<RayList*>& subsets);

	void setup_ispc_pointers();

	void *GetISPC() { return ispc; }

	void print(int which = -1);

	float get_ox(int i);
	float get_oy(int i);
	float get_oz(int i);
	float get_dx(int i);
	float get_dy(int i);
	float get_dz(int i);
	float get_nx(int i);
	float get_ny(int i);
	float get_nz(int i);
	float get_sample(int i);
	float get_r(int i);
	float get_g(int i);
	float get_b(int i);
	float get_o(int i);
	float get_sr(int i);
	float get_sg(int i);
	float get_sb(int i);
	float get_so(int i);
	float get_t(int i);
	float get_tMax(int i);
	int   get_x(int i);
	int   get_y(int i);
	int   get_type(int i);
	int   get_term(int i);

	float* get_ox_base();
	float* get_oy_base();
	float* get_oz_base();
	float* get_dx_base();
	float* get_dy_base();
	float* get_dz_base();
	float* get_nx_base();
	float* get_ny_base();
	float* get_nz_base();
	float* get_sample_base();
	float* get_r_base();
	float* get_g_base();
	float* get_b_base();
	float* get_o_base();
	float* get_sr_base();
	float* get_sg_base();
	float* get_sb_base();
	float* get_so_base();
	float* get_t_base();
	float* get_tMax_base();
	int*   get_x_base();
	int*   get_y_base();
	int*   get_type_base();
	int*   get_term_base();

	void set_ox(int i, float v);
	void set_oy(int i, float v);
	void set_oz(int i, float v);
	void set_dx(int i, float v);
	void set_dy(int i, float v);
	void set_dz(int i, float v);
	void set_nx(int i, float v);
	void set_ny(int i, float v);
	void set_nz(int i, float v);
	void set_sample(int i, float v);
	void set_r(int i, float v);
	void set_g(int i, float v);
	void set_b(int i, float v);
	void set_o(int i, float v);
	void set_sr(int i, float v);
	void set_sg(int i, float v);
	void set_sb(int i, float v);
	void set_so(int i, float v);
	void set_t(int i, float v);
	void set_tMax(int i, float v);
	void set_x(int i, int v);
	void set_y(int i, int v);
	void set_type(int i, int v);
	void set_term(int i, int v);

	RendererP     GetTheRenderer() { return theRenderer; }
	RenderingSetP GetTheRenderingSet() { return theRenderingSet; }
	RenderingP    GetTheRendering() { return theRendering; }

private:
	RendererP theRenderer;
	RenderingP theRendering;
	RenderingSetP theRenderingSet;

	SharedP contents;
	void* ispc;
};

} // namespace gxy

