#pragma once

#include <iostream>
#include <vector>

#include "dtypes.h"
#include "rapidjson/document.h"
#include "ISPCObject.h"

using namespace std;
using namespace rapidjson;

namespace pvol
{
class Lighting : public ISPCObject
{
public:
   Lighting();
  ~Lighting();

  virtual int SerialSize();
  virtual unsigned char *Serialize(unsigned char *);
  virtual unsigned char *Deserialize(unsigned char *);

  void LoadStateFromValue(Value&);
  void SaveStateToValue(Value&, Document&);

  void SetLights(int n, float* l, int* t);
  void GetLights(int& n, float*& l, int*& t);

  void SetK(float ka, float kd);
  void GetK(float& ka, float& kd);

  void SetAO(int n, float r);
  void GetAO(int& n, float& r);

  void SetShadowFlag(bool b);
  void GetShadowFlag(bool& b);

  void SetEpsilon(float e);
  void GetEpsilon(float& e);

protected:
  virtual void allocate_ispc();
  virtual void initialize_ispc();
};
}

