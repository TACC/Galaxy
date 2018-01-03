#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <memory>

#include "KeyedObject.h"

using namespace std;

KEYED_OBJECT_POINTER(Datasets)

#include "KeyedDataObject.h"
#include "Box.h"

#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

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

