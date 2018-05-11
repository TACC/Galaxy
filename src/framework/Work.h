#pragma once 

#include <iostream>
#include <memory>
#include <vector>
#include <mpi.h>
#include "smem.h"

using namespace std;

class Work
{
public:
	Work();
	Work(int n);
	Work(SharedP s);
	Work(const Work* o);
	~Work();

	void *get() { return contents->get(); }
	int   get_size() { return contents->get_size(); }

  virtual void initialize() {};
  virtual void destructor() { std::cerr << "generic Work destructor\n"; }
  virtual bool Action(int sender) { std::cerr << "killed by generic work Action()\n"; return true; };
  virtual bool CollectiveAction(MPI_Comm comm, bool isRoot) { std::cerr << "killed by generic collective work Action()\n"; return true; };

	void Send(int i);
	void Broadcast(bool c, bool b = false);

	string getClassName() { return className; };
  int GetType() { return type; }

	SharedP get_pointer() { return contents; }

	void (*dtor)();

	int xyzzy() { return contents.use_count(); }

protected:
	static int RegisterSubclass(string, Work *(*d)(SharedP));

  int 		type;
  string 	className;
	SharedP contents;
};


#define WORK_CLASS_TYPE(ClassName) 																											\
		int ClassName::class_type = 0;        																							\
		string ClassName::class_name = #ClassName; 																					\


#define WORK_CLASS(ClassName, bcast)				 																						\
 																																												\
public: 																																								\
 																																												\
	ClassName(size_t n) : Work(n)																													\
	{																							 																				\
    className = string(#ClassName); 																										\
    type = ClassName::class_type; 																											\
		initialize(); 																																			\
	} 																																										\
 																																												\
	ClassName(SharedP p) : Work(p) 							  																				\
	{																							 																				\
    className = string(#ClassName); 																										\
    type = ClassName::class_type; 																											\
		initialize(); 																																			\
  } 																																										\
 																																												\
public: 																																								\
	string getClassName() { return string(#ClassName); } 																	\
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
	static string class_name;

