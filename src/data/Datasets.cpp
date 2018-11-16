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

#include "Datasets.h"

#include "Application.h"
#include "KeyedDataObject.h"
#include "Volume.h"
#include "Particles.h"
#include "Triangles.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

namespace gxy
{
	
OBJECT_CLASS_TYPE(Datasets)

void
Datasets::Register()
{
  RegisterClass();
}

void
Datasets::initialize()
{
  keys = NULL;
  super::initialize();
}

Datasets::~Datasets()
{
	if (keys) delete[] keys;
}

void
Datasets::Commit()
{
	for (auto it : datasets)
		it.second->Commit();

	KeyedObject::Commit();
}

void 
Datasets::SaveToJSON(Value& v, Document& doc)
{
  Value d(kArrayType);

	for (auto it : datasets)
	{
		KeyedDataObjectP kdop = it.second;
    kdop->SaveToJSON(d, doc);
	}

	v.AddMember("Datasets", d, doc.GetAllocator());
}

void
Datasets::loadTyped(Value& v)
{
	if (! v.HasMember("filename") || ! v.HasMember("type"))
	{
		std::cerr << "Dataset must have name and type\n";
		exit(0);
	}

	string name(v["filename"].GetString());
	string type(v["type"].GetString());

	KeyedDataObjectP kop;
	if (type == "Particles")
		kop = Particles::NewP();
	else if (type == "Volume")
    kop = Volume::NewP();
	else if (type == "Triangles")
    kop = Triangles::NewP();
	else
	{
		std::cerr << "invalid Dataset type: " << type << "\n";
		exit(0);
	}

	kop->LoadFromJSON(v);
		
	if (v.HasMember("name"))
		name = v["name"].GetString();

	Insert(name.c_str(), kop);
}


void
Datasets::LoadFromJSON(Value& v)
{
	if (! v.HasMember("Datasets"))
	{
		std::cerr << "JSON has no Datasets clause\n";
		exit(0);
	}

	Value& ds = v["Datasets"];

	if (ds.IsArray())
	{
		for (int i = 0; i < ds.Size(); i++)
			loadTyped(ds[i]);
	}
	else
		loadTyped(ds);
}

void
Datasets::LoadFromJSONFile(std::string fname)
{
  Document doc;

  FILE *pFile = fopen (fname.c_str() , "r");
  if (! pFile)
  {
    std::cerr << "Unable to open " << fname << "\n";
  }
  else
  {
    char buf[64];
    rapidjson::FileReadStream is(pFile,buf,64);
    doc.ParseStream<0>(is);
    fclose(pFile);

    LoadFromJSON(doc);
  }
}

void
Datasets::LoadTimestep()
{
	for (auto it : datasets)
	{
		KeyedDataObjectP kdop = it.second;
		if (kdop->is_attached())
			kdop->LoadTimestep();
	}
}

bool
Datasets::WaitForTimestep()
{
	for (auto it : datasets)
	{
		KeyedDataObjectP kdop = it.second;
    if (kdop->is_attached())
    {
      if (! kdop->WaitForTimestep())
        return false;
    }
	}

  return true;
}

bool 
Datasets::IsTimeVarying()
{
	for (auto it : datasets)
		if (it.second->is_time_varying()) return true;
	return false;
}

int
Datasets::serialSize()
{
	return KeyedObject::serialSize() + sizeof(int) + size()*sizeof(Key);
}

unsigned char * 
Datasets::serialize(unsigned char *p)
{
	*(int *)p = size();
	p += sizeof(int);

	for (auto i : datasets)
	{
		*(Key *)p = i.second->getkey();
		p += sizeof(Key);
	}

	return p;
}

unsigned char * 
Datasets::deserialize(unsigned char *p)
{
	nkeys = *(int *)p;
	p += sizeof(int);
	
	keys = new Key[nkeys];
	memcpy((void *)keys, (void *)p, nkeys*sizeof(Key));
	p += nkeys*sizeof(Key);

	return p;
}

}  // namespace gxy
