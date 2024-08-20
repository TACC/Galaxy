// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// ospray
#include "math/vec.ih"
#include "math/box.ih"
#include "common/Ray.ih"
#include "common/Model.ih"
#include "common/safe_gather.ih"
#include "geometry/Geometry.ih"
#include "math/sampling.ih"
#include "transferFunction/TransferFunction.ih"

#include "ospray/OSPDataType.h"
#include "embree3/rtcore.isph"
#include "embree3/rtcore_scene.isph"
#include "embree3/rtcore_common.isph"
#include "embree3/rtcore_geometry.isph"

#define MAX_POINTS 5
#define MAX_PLANES 1

struct points_ 
{
  float x[MAX_POINTS];  
  float y[MAX_POINTS];
  float z[MAX_POINTS];
};

struct sort_element_
{
  float d;
  int   indx;
};

struct queryUserData
{
  RTCScene scene;
  int nPoints;
  struct sort_element_ sort[MAX_POINTS];
	int hits[MAX_POINTS];
};

uniform int max_points = 0;
uniform int max_hits = 0;

struct PointSurface {
  /*! inherit from "Geometry" class: */
  Geometry   super;
  RTCScene   scene;
  vec3f     *points;    /*! data array that contains the points locations */
  float     r0, r1, dr;
};


uniform struct queryUserData sort(uniform struct queryUserData qud)
{
#if 1
  print("PRESORT:\n");
	for (int i = 0; i < qud.nPoints; i++)
		print("%\n", qud.sort[i].d);
#endif
	
  bool done = false;
  while (! done)
  {
    done = true;
    for (int i = 1; i < qud.nPoints; i++)
      if (qud.sort[i].d < qud.sort[i-1].d)
      {
#if 0
				print("SWAP % %\n", i, i-1);
				for (int j = 0; j < qud.nPoints; j++)
					print("%\n", qud.sort[j].d);
#endif
#if 0
				sort_element_ e = qud.sort[i];
				qud.sort[i-1] = qud.sort[i];
				qud.sort[i] = e;
#else
        float d = qud.sort[i].d;
        int indx = qud.sort[i].indx;
        qud.sort[i].d = qud.sort[i-1].d;
        qud.sort[i].indx = qud.sort[i-1].indx;
        qud.sort[i-1].d = d;
        qud.sort[i-1].indx = indx;
#endif
        done = false;
      }
  }

#if 0
  print("POSTSORT:\n");
	for (int i = 0; i < qud.nPoints; i++)
		print("%\n", qud.sort[i].d);
#endif

	return qud;
}

static void PointSurface_postIntersect(uniform Geometry *uniform geometry,
                                  uniform Model *uniform model,
                                  varying DifferentialGeometry &dg,
                                  const varying Ray &ray,
                                  uniform int64 flags)
{

  uniform PointSurface *uniform self = (uniform PointSurface *uniform)geometry;
  dg.Ng = dg.Ns = ray.Ng;

  //vec3f color = self->transferFunction->getColorForValue(self->transferFunction, dataval);

  dg.color.x = 1.0;
  dg.color.y = 1.0;
  dg.color.z = 1.0;
  dg.color.w = 1.0;
}

unmasked void PointSurface_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  uniform PointSurface *uniform self = (uniform PointSurface *uniform)args->geometryUserPtr;
  uniform int primID = args->primID;
  
  uniform vec3f point = self->points[primID];

  box3fa *uniform out = (box3fa *uniform)args->bounds_o;
  *out = make_box3fa(point - self->r0, point + self->r0);

}

unmasked bool 
PointSurface_PointQueryCallback(RTCPointQueryFunctionArguments *uniform a)
{   
  uniform struct queryUserData *uniform u = (uniform struct queryUserData *uniform)a->userPtr;

  RTCScene scene = u->scene;

  RTCGeometry geom = rtcGetGeometry(scene, a->geomID);
  uniform PointSurface *uniform self = (uniform PointSurface *uniform)rtcGetGeometryUserData(geom);
  uniform int primID = a->primID;

  if (u->nPoints < MAX_POINTS)
	{
    u->hits[u->nPoints++] = a->primID;
	}
  else
    a->query->radius = 0;
}

void PointSurface_intersect_kernel(const RTCIntersectFunctionNArguments *uniform args,
                              const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex]) return;

#if 0
  print("VALID: ");
  for (int i = 0; i < 8; i++)
    print("% \n", args->valid[i]);
  print("\n");
#endif

  uniform PointSurface *uniform self = (uniform PointSurface *uniform)args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  vec3f point = self->points[primID]; 
  
  RTCScene scene = self->scene;
  uniform RTCPointQueryContext ctxt;
  rtcInitPointQueryContext(&ctxt);

	uniform queryUserData	qud;
	qud.nPoints = 0;
	qud.scene = scene;
	void *userPtr = (void *)&qud;

  RTCPointQuery q; 

  varying Ray *uniform ray = (varying Ray *uniform)args->rayhit;
  varying RTCRayHit *uniform rayHit = (varying RTCRayHit *uniform)args->rayhit;

  // guess distance as the distance from the primitive point to the ray origin

  vec3f v = point - ray->org;
  vec3f p = ray->org + sqrt(dot(v,v))*ray->dir;

	print("RAY ORIGIN % % %\n", ray->org.x, ray->org.y, ray->org.z);
	print("RAY DIR % % %\n", ray->dir.x, ray->dir.y, ray->dir.z);
	print("TEST POINT % % %\n", p.x, p.y, p.z);

  q.x = p.x;
  q.y = p.y;
  q.z = p.z;

  q.radius = self->r1;

  rtcPointQueryV(scene, &q, &ctxt, PointSurface_PointQueryCallback, &userPtr); 

	// Get the distance from the points to the query point so we can sort

	for (int i = 0; i < qud.nPoints; i++)
	{
		vec3f d = self->points[qud.hits[i]] - point;
		qud.sort[i].d = sqrt(dot(d,d));
		qud.sort[i].indx = i;
	}

	qud = sort(qud);

	print("SORTS\n");
	for (int i = 0; i < qud.nPoints; i++)
		print("% %\n", qud.sort[i].indx, qud.sort[i].d);

  bool hit = true;
  int nHits = 0;
  float T = 0;

  vec3f hitPoint, hitNormal;

	if (qud.nPoints > max_points)
		max_points = reduce_max(qud.nPoints);

	vec3f org = ray->org;
	vec3f dir = ray->dir;

	int nh = 0;  // Number of triangle hits in each lane

	vec3f normal_sum = make_vec3f(0.0, 0.0, 0.0);
	vec3f point_sum = make_vec3f(0.0, 0.0, 0.0);

	int JJ, KK, LL;

	for (int j = 0; j < qud.nPoints - 2 && nh < MAX_PLANES; j++)
	{
		int p = qud.hits[qud.sort[j].indx];
		vec3f P = self->points[p];

		for (int k = j + 1; k < qud.nPoints -  1 && nh < MAX_PLANES; k++)
		{
			int q = qud.hits[qud.sort[k].indx];
			vec3f Q = self->points[q];
			vec3f PQ = Q - P;

			for (int l = k + 1; l < qud.nPoints && nh < MAX_PLANES; l++)
			{

				int r = qud.hits[qud.sort[l].indx];
				vec3f R = self->points[r];
				vec3f QR = R - Q;
				vec3f RP = P - R;

				// print("TI %\nTJ %\nTK %\n", p, q, r);

				//print("P % % %\n", P.x, P.y, P.z);
				//print("Q % % %\n", Q.x, Q.y, Q.z);
				//print("R % % %\n", R.x, R.y, R.z);
				//print("QR % % %\n", QR.x, QR.y, QR.z);
				//print("RP % % %\n", RP.x, RP.y, RP.z);

				// Triangle is zero area of two edges are collinear

        float degenerate = dot(normalize(QR), normalize(RP));
				if (degenerate > 0.999 || degenerate < -0.999)
				{
					continue;
				}

				bool hit = false;
        vec3f normal = normalize(cross(PQ, QR));

				// print("PX %\nPY %\nPZ %\n", P.x, P.y, P.z);
				// print("QX %\nQY %\nQZ %\n", Q.x, Q.y, Q.z);
				// print("RX %\nRY %\nRZ %\n", R.x, R.y, R.z);
				// print("NX %\nNY %\nNZ %\n", normal.x, normal.y, normal.z);

				if (dot(normal, dir) != 0)
				{
					float D = -dot(normal, P);
					float t = -(dot(normal, org) + D) / dot(normal, dir);
					if (t > 0)
					{
						vec3f pt = org + t*dir;  // hit point in plane of triangle PQR
              
						if (dot(pt - P, PQ) > 0)
							hit = (dot(pt - Q, QR) > 0) && (dot(pt - R, RP) > 0);
						else
							hit = (dot(pt - Q, QR) < 0) && (dot(pt - R, RP) < 0);
						
						if (hit)
						{
							JJ = j;
							KK = k;
							LL = l;
							if (dot(normal, dir) < 0)
								normal_sum = normal_sum + normal;
							else
								 normal_sum = normal_sum - normal;

							point_sum = point_sum + pt;
							nh = nh + 1;
            }
          }
        }
      }
    }
	}

	print("TRIANGLES % % %\n", JJ, KK, LL);

	if (nh > max_hits)
	{
		//print("inserting % at %\n", nh, i);
//		print("H %\n", nh);
		max_hits = reduce_max(nh);
	}

	if (nh > 0)
	{
		hitPoint = point_sum / nh;
		hitNormal = normalize(normal_sum);

		vec3f v = hitPoint - org;

		//print("PS % % %\n", point_sum.x, point_sum.y, point_sum.z);
		//print("ORG % % %\n", org.x, org.y, org.z);
		//print("% % %\n", normal_sum.x, normal_sum.y, normal_sum.z);

		T = sqrt(dot(v, v));
  }

  if (nh)
  {
    // print("nHits %\n", nh);
    vec3f v = hitPoint - ray->org;
    ray->t = T;
    ray->Ng = hitNormal;
//		print("RX %\nRY %\nRZ %\n", hitNormal.x, hitNormal.y, hitNormal.z);
    ray->geomID = 0;
  }
}

unmasked void PointSurface_intersect(const struct RTCIntersectFunctionNArguments *uniform args)
{
  PointSurface_intersect_kernel(args,false);
}

unmasked void PointSurface_occluded(const struct RTCIntersectFunctionNArguments *uniform args)
{
  PointSurface_intersect_kernel(args,true);
}

export void *uniform PointSurface_create(void *uniform cppEquivalent)
{
  uniform PointSurface *uniform self = uniform new uniform PointSurface;
  Geometry_Constructor(&self->super,cppEquivalent,
                       PointSurface_postIntersect,
                       NULL,
                       NULL,
                       0,NULL);
  return self;
}

export void PointSurfaceGeometry_set(void  *uniform _self
                                , void *uniform _model
                                , void *uniform points
                                , uniform int numPoints
                                , uniform float r0
                                , uniform float r1
                                , uniform float dr
                                , float *uniform box
                                )
{
  uniform PointSurface *uniform self = (uniform PointSurface *uniform)_self;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  RTCGeometry geom = rtcNewGeometry(ispc_embreeDevice(),RTC_GEOMETRY_TYPE_USER);
  uniform uint32 geomID = rtcAttachGeometry(model->embreeSceneHandle,geom);

  self->super.model = model;
  self->super.geomID = geomID;
  self->super.numPrimitives = numPoints;
  self->scene = model->embreeSceneHandle;

  self->points = (vec3f *uniform)points;
  uniform int numPoints = self->super.numPrimitives;

  self->r0 = r0;
  self->r1 = r1;
  self->dr = dr;

  vec3f minb, maxb;
  uniform int isFirst = 1;

  foreach (i = 0 ... numPoints)
  {
    vec3f p = self->points[i];
    float radius;

    if (isFirst)
    {
      isFirst = 0;
      minb = p;
      maxb = p;
    }
    else
    {
      if (minb.x > p.x) minb.x = p.x;
      if (minb.y > p.y) minb.y = p.y;
      if (minb.z > p.z) minb.z = p.z;

      if (maxb.x < p.x) maxb.x = p.x;
      if (maxb.y < p.y) maxb.y = p.y;
      if (maxb.z < p.z) maxb.z = p.z;
    }
  }
  box[0] = reduce_min(minb.x);
  box[1] = reduce_min(minb.y);
  box[2] = reduce_min(minb.z);
  box[3] = reduce_max(maxb.x);
  box[4] = reduce_max(maxb.y);
  box[5] = reduce_max(maxb.z);

  rtcSetGeometryUserData(geom, self);
  rtcSetGeometryUserPrimitiveCount(geom,numPoints);
  rtcSetGeometryBoundsFunction
    (geom,(uniform RTCBoundsFunction)&PointSurface_bounds, self);
  rtcSetGeometryIntersectFunction
    (geom,(uniform RTCIntersectFunctionN)&PointSurface_intersect);
  rtcSetGeometryOccludedFunction
    (geom,(uniform RTCOccludedFunctionN)&PointSurface_occluded);
  rtcCommitGeometry(geom);
  rtcReleaseGeometry(geom);
}
