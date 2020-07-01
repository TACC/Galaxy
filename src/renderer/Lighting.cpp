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

#include <math.h>

#include "Application.h"
#include "KeyedObject.h"
#include "Lighting.h"
#include "Lighting_ispc.h"

using namespace rapidjson;
using namespace std;

namespace gxy
{

Lighting::Lighting()
{
  allocate_ispc();
  initialize_ispc();
	set = false;
}

Lighting::~Lighting()
{
  destroy_ispc();
}

void
Lighting::allocate_ispc()
{
  ispc = ispc::Lighting_allocate();
}

void
Lighting::initialize_ispc()
{
  ispc::Lighting_initialize(GetIspc());
}

void
Lighting::LoadStateFromValue(Value& v)
{
	set = true;
  if (v.HasMember("Sources"))
  {
    Value& s = v["Sources"];

    float *lights = new float[s.Size() * 3];
    int *types = new int[s.Size()];

    for (int i = 0; i < s.Size(); i++)
    {
      float x = s[i][0].GetDouble();
      float y = s[i][1].GetDouble();
      float z = s[i][2].GetDouble();
			int t = s[i].Size() == 3 ? 0 : s[i][3].GetInt();

			if (t == 0)
			{
				float d = sqrt(x*x + y*y + z*z);
				if (d == 0.0)
				{
					std::cerr << "WARNING: Directional light souce cannot be (0,0,0) so using (1,1,1)" << std::endl;
					x = y = z = 0.577350;
				}
				else
				{
					x = x / d;
					y = y / d;
					z = z / d;
				}
			}

      lights[i*3 + 0] = x;
      lights[i*3 + 1] = y;
      lights[i*3 + 2] = z;
			types[i] = t;
    }

    SetLights(s.Size(), lights, types);
    delete[] lights;
    delete[] types;
  }

  if (v.HasMember("shadows"))
    SetShadowFlag(v["shadows"].GetBool());

  int   aok = v.HasMember("ao count") ? v["ao count"].GetInt() : 0;
  float aor = v.HasMember("ao radius") ? v["ao radius"].GetDouble() : 1;
    
  SetAO(aok, aor);

  float ka = v.HasMember("Ka") ? v["Ka"].GetDouble() : 0.5;
  float kd = v.HasMember("Kd") ? v["Kd"].GetDouble() : 0.5;
  
  SetK(ka, kd);
}

void
Lighting::SaveStateToValue(Value& v, Document& doc)
{
  Value lv;

  {
    Value l(kArrayType);
    int n; float *lptr; int *t;
    GetLights(n, lptr, t);
    for (int i = 0; i < n; i++)
    {
      Value ll(kArrayType);
      ll.PushBack(Value().SetDouble(*lptr++), doc.GetAllocator());
      ll.PushBack(Value().SetDouble(*lptr++), doc.GetAllocator());
      ll.PushBack(Value().SetDouble(*lptr++), doc.GetAllocator());
      ll.PushBack(Value().SetInt(*lptr++), doc.GetAllocator());
      l.PushBack(ll, doc.GetAllocator());
    }
    lv.AddMember("Sources", l, doc.GetAllocator());
  }

  {
    Value k(kArrayType);
    float ka, kd;
    GetK(ka, kd);
    k.PushBack(Value().SetDouble(ka), doc.GetAllocator());
    k.PushBack(Value().SetDouble(kd), doc.GetAllocator());
    lv.AddMember("K", k, doc.GetAllocator());
  }

  {
    bool b;
    GetShadowFlag(b);
    lv.AddMember("ShadowFlag", Value().SetBool(b), doc.GetAllocator());
  }

  {
    float ar; int na;
    GetAO(na, ar);
    Value AO(kArrayType);
    AO.PushBack(Value().SetInt(na), doc.GetAllocator());
    AO.PushBack(Value().SetDouble(ar), doc.GetAllocator());
    lv.AddMember("AO", AO, doc.GetAllocator());
  }

  v.AddMember("Lighting", lv, doc.GetAllocator());
}

int
Lighting::SerialSize()
{
  int n; float *p; int *t;
  GetLights(n, p, t);
  return 4*sizeof(int) +     								// n_ao_rays, do_shadows, nLights
         4*sizeof(float) +   								// Ka, Kd, ao_radius, epsilon
         n*(sizeof(vec3f) + sizeof(int));   // the lights
}

unsigned char *
Lighting::Serialize(unsigned char *p)
{
  {
    int na; float nr;
    GetAO(na, nr);
    *(int *)p = na;
    p += sizeof(int);
    *(float *)p = nr;
    p += sizeof(float);
  }

  {
    int b; bool bb;
    GetShadowFlag(bb);
    *(int *)p = bb ? 1 : 0;
    p += sizeof(int);
  }

  {
    float ka, kd;
    GetK(ka, kd);
    *(float *)p = ka;
    p += sizeof(float);
    *(float *)p = kd;
    p += sizeof(float);
  }

  {
    int n; float *l; int *t;
    GetLights(n, l, t);
    *(int *)p = n;
    p += sizeof(int);
    memcpy(p, l, n*sizeof(vec3f));
    p += n*sizeof(vec3f);
    memcpy(p, t, n*sizeof(int));
    p += n*sizeof(int);
  }

  return p;
}

unsigned char *
Lighting::Deserialize(unsigned char *p)
{
  SetAO(*(int *)p, *(float *)(p + sizeof(int)));
  p = p + sizeof(float) + sizeof(int);

  SetShadowFlag(*(int *)p);
  p = p + sizeof(int);

  SetK(*(float *)p, *(float *)(p + sizeof(float)));
  p += 2*sizeof(float);

  int n = *(int *)p;
  p += sizeof(int);

	float *l = (float *)p;
	p += n*sizeof(vec3f);

	int *t = (int *)p;
	p += n*sizeof(int);

  SetLights(n, (float *)l, t);

  return p;
}

void
Lighting::SetLights(int n, float *l, int *t)
{
  ispc::Lighting_SetLights(GetIspc(), n, l, t);
}

void 
Lighting::GetLights(int& n, float*& l, int*& t)
{
  ispc::Lighting_GetLights(GetIspc(), &n, &l, &t);
}

void
Lighting::SetK(float ka, float kd)
{
  ispc::Lighting_SetK(GetIspc(), ka, kd);
}
 
void
Lighting::GetK(float& ka, float& kd)
{
  ispc::Lighting_GetK(GetIspc(), &ka, &kd);
}
 
void
Lighting::SetAO(int n, float r)
{
  ispc::Lighting_SetAO(GetIspc(), n, r);
}
 
void
Lighting::GetAO(int& n, float& r)
{
  ispc::Lighting_GetAO(GetIspc(), &n, &r);
}
 
void
Lighting::SetShadowFlag(bool b)
{
  ispc::Lighting_SetShadowFlag(GetIspc(), b ? 1 : 0);
}
 
void
Lighting::GetShadowFlag(bool &b)
{
  int bb;
  ispc::Lighting_GetShadowFlag(GetIspc(), &bb);
  b = bb == 1;
}
 
} // namespace gxy

