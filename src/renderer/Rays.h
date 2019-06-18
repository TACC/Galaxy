// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
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

/*! \file Rays.h 
 * \brief a list of rays, the primary work unit in Galaxy's renderer
 * \ingroup render
 */

#include <memory>
#include <vector>
#include "Rendering.h"
#include "smem.h"

namespace gxy
{

OBJECT_POINTER_TYPES(Rendering)
OBJECT_POINTER_TYPES(RenderingSet)

//! a list of rays, the primary work unit in Galaxy's renderer
/*! \sa KeyedObject, Rendering, RenderingSet
 * \ingroup render
 */
class RayList 
{
public:
	enum RayListType { PRIMARY, SECONDARY }; //!< ray types, used to specify handling (e.g. in priority queue)

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
	~RayList(); //!< default destructor

	//! deep copy the ray at the given position in the source RayList to the given position in the destination RayList
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
    dstRayList->set_classification(dstRayIndex, srcRayList->get_classification(srcRayIndex));
	}

	RayList(RendererP renderer, RenderingSetP rs, RenderingP r, int nrays, int frame, RayListType type); //!< constructur
	RayList(RendererP renderer, RenderingSetP rs, RenderingP r, int nrays, RayListType type); //!< constructor
	RayList(SharedP contents); //!< constructor from ISPC serialization

	RayListType  GetType() { return ((struct hdr *)contents->get())->type; } //!< get the type of rays in this RayList
	void SetType(RayListType t) { ((struct hdr *)contents->get())->type = t; }; //!< set the type of rays in this RayList

	int GetFrame() { return ((struct hdr *)contents->get())->frame; } //!< returns which frame this RayList renders into
	int GetRayCount() { return ((struct hdr *)contents->get())->size; } //!< returns the number of rays in this RayList
	int GetId() { return ((struct hdr *)contents->get())->id; } //!< return the pixel id this RayList renders into
	SharedP get_ptr() { return contents; }; //!< returns a shared pointer to the ISPC contents of this ray list

	//! returns a pointer to the header of the ISPC contents of this RayList
	void *get_header_address() { return ((void *)(contents->get())); } 

	//! set this RayList to hold the given number of rays
	/*! This method changes the capacity of this RayList to the given value *n*. 
	 * If *n* is larger than the current capacity, the capacity is increased to *n*.
	 * If *n* is smaller than the current capacity, the first *n* rays are kept in the RayList.
	 * \warning if *n* is not a multiple of 16, *n* is increased to the next multiple of 16 to preserve instruction alignment.
	 * \param n the new size of this RayList
	 */
	void Truncate(int n);
	//! subdivide this RayList into maximum RayList sized subsets
	/* \sa Renderer::GetMaxRayListSize()
	 * This method breaks this RayList into one or more subsets, 
	 * where each subset will have the maximum number of rays as set in the Renderer.
	 * The last RayList of the set contains any remaining rays, and thus may have fewer than the maximum number.
	 */
	void Split(std::vector<RayList*>& subsets);

	//! configure pointers for the ISPC representation of this RayList
	void setup_ispc_pointers();

	//! return a pointer to the ISPC context for this RayList
	void *GetIspc() { return ispc; }

	//! print a certain ray or the entire RayList
	/* by default, this method prints the entire RayList. 
	 * If an index is specified, this method prints only the ray at that index.
	 * \warning no range checking is performed. Out-of-range access will likely segfault.
	 */
	void print(int which = -1); 

	float get_ox(int i); //!< return the x component of the ray origin for the ith ray in the RayList
	float get_oy(int i); //!< return the y component of the ray origin for the ith ray in the RayList
	float get_oz(int i); //!< return the z component of the ray origin for the ith ray in the RayList
	float get_dx(int i); //!< return the x component of the ray direction for the ith ray in the RayList
	float get_dy(int i); //!< return the y component of the ray direction for the ith ray in the RayList
	float get_dz(int i); //!< return the z component of the ray direction for the ith ray in the RayList
	float get_nx(int i); //!< return the x component of the ray normal for the ith ray in the RayList
	float get_ny(int i); //!< return the y component of the ray normal for the ith ray in the RayList
	float get_nz(int i); //!< return the z component of the ray normal for the ith ray in the RayList
	float get_sample(int i); //!< return the sample value for the ith ray in the RayList
	float get_r(int i); //!< return the r component of the accumulated ray color for the ith ray in the RayList
	float get_g(int i); //!< return the g component of the accumulated ray color for the ith ray in the RayList
	float get_b(int i); //!< return the b component of the accumulated ray color for the ith ray in the RayList
	float get_o(int i); //!< return the o component of the accumulated ray color for the ith ray in the RayList
	float get_sr(int i); //!< return the r component of the intersected surface color for the ith ray in the RayList
	float get_sg(int i); //!< return the g component of the intersected surface color for the ith ray in the RayList
	float get_sb(int i); //!< return the b component of the intersected surface color for the ith ray in the RayList
	float get_so(int i); //!< return the o component of the intersected surface color for the ith ray in the RayList
	float get_t(int i); //!< return the t component for the ith ray in the RayList
	float get_tMax(int i); //!< return the tMax component for the ith ray in the RayList
	int   get_x(int i); //!< return the x index of the pixel for the ith ray in the RayList
	int   get_y(int i); //!< return the y index of the pixel for the ith ray in the RayList
	int   get_type(int i); //!< return the ray type for the ith ray in the RayList
	int   get_term(int i); //!< return whether the ray has terminated for the ith ray in the RayList
	int   get_classification(int i); //!< return the classification of the ith ray in the RayList

	float* get_ox_base(); //!< get a pointer to the head of the origin x array for this RayList
	float* get_oy_base(); //!< get a pointer to the head of the origin y array for this RayList
	float* get_oz_base(); //!< get a pointer to the head of the origin z array for this RayList
	float* get_dx_base(); //!< get a pointer to the head of the direction x array for this RayList
	float* get_dy_base(); //!< get a pointer to the head of the direction y array for this RayList
	float* get_dz_base(); //!< get a pointer to the head of the direction z array for this RayList
	float* get_nx_base(); //!< get a pointer to the head of the normal x array for this RayList
	float* get_ny_base(); //!< get a pointer to the head of the normal y array for this RayList
	float* get_nz_base(); //!< get a pointer to the head of the normal z array for this RayList
	float* get_sample_base(); //!< get a pointer to the head of the sample array for this RayList
	float* get_r_base(); //!< get a pointer to the head of the accumulated color r array for this RayList
	float* get_g_base(); //!< get a pointer to the head of the accumulated color g array for this RayList
	float* get_b_base(); //!< get a pointer to the head of the accumulated color b array for this RayList
	float* get_o_base(); //!< get a pointer to the head of the accumulated color o array for this RayList
	float* get_sr_base(); //!< get a pointer to the head of the intersected surface color r array for this RayList
	float* get_sg_base(); //!< get a pointer to the head of the intersected surface color g array for this RayList
	float* get_sb_base(); //!< get a pointer to the head of the intersected surface color b array for this RayList
	float* get_so_base(); //!< get a pointer to the head of the intersected surface color o array for this RayList
	float* get_t_base(); //!< get a pointer to the head of the t array for this RayList
	float* get_tMax_base(); //!< get a pointer to the head of the tMax array for this RayList
	int*   get_x_base(); //!< get a pointer to the head of the pixel x array for this RayList
	int*   get_y_base(); //!< get a pointer to the head of the pixel y array for this RayList
	int*   get_type_base(); //!< get a pointer to the head of the type array for this RayList
	int*   get_term_base(); //!< get a pointer to the head of the termination array for this RayList
	int*   get_classification_base(); //!< get a pointer to the head of the classification array for this RayList

	void set_ox(int i, float v); //!< set the x component of the ray origin for the ith ray in the RayList
	void set_oy(int i, float v); //!< set the y component of the ray origin for the ith ray in the RayList
	void set_oz(int i, float v); //!< set the z component of the ray origin for the ith ray in the RayList
	void set_dx(int i, float v); //!< set the x component of the ray direction for the ith ray in the RayList
	void set_dy(int i, float v); //!< set the y component of the ray direction for the ith ray in the RayList
	void set_dz(int i, float v); //!< set the z component of the ray direction for the ith ray in the RayList
	void set_nx(int i, float v); //!< set the x component of the ray normal for the ith ray in the RayList
	void set_ny(int i, float v); //!< set the y component of the ray normal for the ith ray in the RayList
	void set_nz(int i, float v); //!< set the z component of the ray normal for the ith ray in the RayList
	void set_sample(int i, float v); //!< set the sample value for the ith ray in the RayList
	void set_r(int i, float v); //!< set the r component of the accumulated ray color for the ith ray in the RayList
	void set_g(int i, float v); //!< set the g component of the accumulated ray color for the ith ray in the RayList
	void set_b(int i, float v); //!< set the b component of the accumulated ray color for the ith ray in the RayList
	void set_o(int i, float v); //!< set the o component of the accumulated ray color for the ith ray in the RayList
	void set_sr(int i, float v); //!< set the r component of the intersected surface color for the ith ray in the RayList
	void set_sg(int i, float v); //!< set the g component of the intersected surface color for the ith ray in the RayList
	void set_sb(int i, float v); //!< set the b component of the intersected surface color for the ith ray in the RayList
	void set_so(int i, float v); //!< set the o component of the intersected surface color for the ith ray in the RayList
	void set_t(int i, float v); //!< set the t component for the ith ray in the RayList
	void set_tMax(int i, float v); //!< set the tMax component for the ith ray in the RayList
	void set_x(int i, int v); //!< set the x index of the pixel for the ith ray in the RayList
	void set_y(int i, int v); //!< set the y index of the pixel for the ith ray in the RayList
	void set_type(int i, int v); //!< set the ray type for the ith ray in the RayList
	void set_term(int i, int v); //!< set whether the ray has terminated for the ith ray in the RayList
	void set_classification(int i, int c); //!< set the classification of the ray

	RendererP     GetTheRenderer() { return theRenderer; } //!< get a pointer to the Renderer for this RayList
	RenderingSetP GetTheRenderingSet() { return theRenderingSet; } //!< get a pointer to the RenderingSet for this RayList
	RenderingP    GetTheRendering() { return theRendering; } //!< get a pointer to the Rendering for this RayList

private:
	RendererP theRenderer;
	RenderingP theRendering;
	RenderingSetP theRenderingSet;

	SharedP contents;
	void* ispc;
};

} // namespace gxy

