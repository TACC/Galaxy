#pragma once

#include <string>
#include <string.h>
#include <vector>
#include <memory>

using namespace std;

namespace pvol 
{

class ISPCObject 
{
public:
  ISPCObject() { ispc = NULL; }
  virtual ~ISPCObject() { destroy_ispc(); }

  void *GetISPC() { return ispc; }

protected:
  virtual void allocate_ispc();
  virtual void initialize_ispc();
  virtual void destroy_ispc();

  void *ispc;
};

}
