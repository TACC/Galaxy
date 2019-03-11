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

/*! \file Work.h 
 * \brief the base class for all Message payloads in Galaxy
 * \ingroup framework
 */

#include <iostream>
#include <memory>
#include <mpi.h>
#include <vector>

#include "smem.h"

namespace gxy
{

//! the base class for all Message payloads in Galaxy
/*! \ingroup framework */
class Work
{
public:
	Work(); //!< default constructor
	Work(int n); //!< construct a Work object with `n` bytes of shareable memory 
	Work(SharedP s); //!< construct a Work object with the given SharedP as contents
	Work(const Work* o); //!< copy constructor
	~Work();  //!< default destructor

	//! return the contents of this Work object 
	void *get() { return contents->get(); }
	//! get the size of the contents of this Work object
	int   get_size() { return contents->get_size(); }

	//! default initialization (no-op)
  virtual void initialize() {};
  //! default de-initialization (no-op)
  virtual void destructor() { std::cerr << "generic Work destructor" << std::endl; }
  //! perform a non-collective action, as defined by the derived class (base is no-op)
  virtual bool Action(int sender) 
  { std::cerr << "killed by generic work Action()" << std::endl; return true; };
  //! perform a collective action, as defined by the derived class (base is no-op)
  virtual bool CollectiveAction(MPI_Comm comm, bool isRoot) 
  { std::cerr << "killed by generic collective work Action()" << std::endl; return true; };

  //! send this Work to the process rank `i`
	void Send(int i);

	//! broadcast this work to all processes
	/*! \param c should this be collective (i.e. synchronizing, run in the message thread with an available MPI communicator)?
	 *  \param b should the sender block until the broadcast is complete?
	 */
	void Broadcast(bool c, bool b = false);

	//! get the class name for this Work (should be set by derived classes)
	std::string getClassName() { return className; };
	//! get the type for this Work (should be set by derived classes)
  int GetType() { return type; }

  //! returns a shared pointer to the contents of this Work
	SharedP get_pointer() { return contents; }

	//! function pointer for custom destructor
	void (*dtor)();

	//! returns the reference count for the shared pointer contents of this Work
	int xyzzy() { return contents.use_count(); }

protected:
	static int RegisterSubclass(std::string, Work *(*d)(SharedP));

  int 		      type;
  std::string 	className;
	SharedP       contents;
};

//! defines a registry tracker for a Work-derived class 
/*! \param ClassName the class name that has Work as an ancestor class
 * \ingroup framework
 */
#define WORK_CLASS_TYPE(ClassName) 																											\
		int ClassName::class_type = 0;        																							\
		std::string ClassName::class_name = #ClassName; 							  										\


//! provides class members for a class that derives from Work
/*! \param ClassName the class name that has Work as an ancestor class
 * \param bcast *unused*
 * \ingroup framework
 */
#define WORK_CLASS(ClassName, bcast)				 																						\
 																																												\
public: 																																								\
 																																												\
	ClassName(size_t n = 0) : Work(n)																													\
	{																							 																				\
    className = std::string(#ClassName); 																								\
    type = ClassName::class_type; 																											\
		initialize(); 																																			\
	} 																																										\
 																																												\
	ClassName(SharedP p) : Work(p) 							  																				\
	{																							 																				\
    className = std::string(#ClassName); 																								\
    type = ClassName::class_type; 																											\
		initialize(); 																																			\
  } 																																										\
 																																												\
public: 																																								\
	std::string getClassName() { return std::string(#ClassName); } 												\
	unsigned char *get_contents() { return contents->get(); } 														\
 																																												\
	static void Register()   																															\
	{																							 																				\
    ClassName::class_type = Work::RegisterSubclass(ClassName::class_name, Deserialize);	\
  } 																																										\
																																												\
  static Work *Deserialize(SharedP ptr)   																							\
	{																							 																				\
		return (Work *) new ClassName(ptr); 																								\
  } 																																										\
																																												\
private: 																																								\
	static int class_type;																																\
	static std::string class_name;

} // namespace gxy
