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

/*! \file Datasets.h 
 * \brief container for KeyedDataObjects within Galaxy
 * \ingroup data
 */

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <memory>

#include "KeyedObject.h"
#include "KeyedDataObject.h"
#include "Box.h"

#include "rapidjson/document.h"

namespace gxy
{

OBJECT_POINTER_TYPES(Datasets)

//! container for KeyedDataObjects within Galaxy
/*! \ingroup data 
 * \sa KeyedObject, KeyedDataObject
 */
class Datasets : public KeyedDataObject
{
  KEYED_OBJECT_SUBCLASS(Datasets, KeyedDataObject)

	using datasets_t = std::map<std::string, KeyedDataObjectP>;

public:
	void initialize(); //!< initialize this Datasets object
	virtual	~Datasets(); //!< default destructor

	//! commit this object to the global registry across all processes
	virtual void Commit();

	//! add the given KeyedDataObject to this Datasets object
	/* \param name the name for the KeyedDataObject
	 * \param val a pointer to the KeyedDataObject
	 */
  void Insert(std::string name, KeyedDataObjectP val) 
  {
  	datasets.insert(std::pair<std::string, KeyedDataObjectP>(name, val));
  }

  //! find the Key for a KeyedDataObject in this Datasets
  /*! \param name the name of the desired KeyedDataObject
   * \returns the Key for the KeyedDataObject with the given name, 
   *          or `-1` if no such object exists
   */
	Key FindKey(std::string name)
	{
		KeyedDataObjectP kdop = Find(name);
		if (kdop != NULL)
			return kdop->getkey();
		else
			return (Key)-1;
	}

	//! return a std::vector containing the names of data in this Datasets object
	std::vector<std::string> GetDatasetNames()
	{
		std::vector<std::string> v;
		for (auto a : datasets)
			v.push_back(a.first);
		return v;
	}
			
  //! find a KeyedDataObject in this Datasets
  /*! \param name the name of the desired KeyedDataObject
   * \returns a pointer to the KeyedDataObject with the given name, 
   *          or `NULL` if no such object exists
   */
  KeyedDataObjectP Find(std::string name)
  {
  	std::map<std::string, KeyedDataObjectP>::iterator it = datasets.find(name);
  	if (it == datasets.end()) return NULL;
  	else return (*it).second;
  }

  //! delete a data object
  /*! \param name the name of the dataset to drop
   */
  void DropDataset(std::string name)
  {
    if (datasets.find(name) == datasets.end())
      std::cerr  << "no dataset named " << name << "\n";
    else
      datasets.erase(name);
  }

  //! load from a Galaxy JSON specification
  virtual void  LoadFromJSON(rapidjson::Value&);

  //! save this Datasets to a Galaxy JSON specification 
  virtual void  SaveToJSON(rapidjson::Value&, rapidjson::Document&);

  //! load from a file containing a JSON spec
  virtual void LoadFromJSONFile(std::string);

	bool IsTimeVarying(); //!< are the data in this Datasets time-varying?
  bool WaitForTimestep(); //!< wait for receipt of next timestep for all attached data sources (e.g. a running simulation for in situ analysis)
  void LoadTimestep(); //!< broadcast a LoadTimestepMsg to all Galaxy processes for all attached data sources (e.g. a running simulation for in situ analysis)

	using iterator = datasets_t::iterator;
	//! return an iterator positioned at the beginning of the data list for this Datasets
  iterator begin() { return datasets.begin(); }
	//! return an iterator positioned at the end of the data list for this Datasets
  iterator end() { return datasets.end(); }
  //! return the number of data objects in this Datasets
  size_t size() { return datasets.size(); }

protected:

  virtual int serialSize();
  virtual unsigned char *serialize(unsigned char *ptr);
  virtual unsigned char *deserialize(unsigned char *ptr);

	void loadTyped(rapidjson::Value& v);

  datasets_t datasets;

	int nkeys;
	Key *keys;
};


} // namespace gxy

