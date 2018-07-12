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

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <memory>

#include "KeyedObject.h"
#include "KeyedDataObject.h"
#include "Box.h"
#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;


namespace gxy
{

KEYED_OBJECT_POINTER(Datasets)

class Datasets : public KeyedDataObject
{
  KEYED_OBJECT_SUBCLASS(Datasets, KeyedDataObject)

	using datasets_t = map<string, KeyedDataObjectP>;

public:
	void initialize();
	virtual	~Datasets();

	virtual void Commit();

  void Insert(string name, KeyedDataObjectP val) 
  {
  	datasets.insert(pair<string, KeyedDataObjectP>(name, val));
  }

	Key FindKey(string name)
	{
		KeyedDataObjectP kdop = Find(name);
		if (kdop != NULL)
			return kdop->getkey();
		else
			return (Key)-1;
	}

	vector<string> GetDatasetNames()
	{
		vector<string> v;
		for (auto a : datasets)
			v.push_back(a.first);
		return v;
	}
			
  KeyedDataObjectP Find(string name)
  {
  	map<string, KeyedDataObjectP>::iterator it = datasets.find(name);
  	if (it == datasets.end()) return NULL;
  	else return (*it).second;
  }

  virtual void  LoadFromJSON(Value&);
  virtual void  SaveToJSON(Value&, Document&);

	bool IsTimeVarying();
  bool WaitForTimestep();
  void LoadTimestep();

	using iterator = datasets_t::iterator;
  iterator begin() { return datasets.begin(); }
  iterator end() { return datasets.end(); }
  size_t size() { return datasets.size(); }

protected:

  virtual int serialSize();
  virtual unsigned char *serialize(unsigned char *ptr);
  virtual unsigned char *deserialize(unsigned char *ptr);

	void loadTyped(Value& v);

  datasets_t datasets;

	int nkeys;
	Key *keys;
};
} // namespace gxy
