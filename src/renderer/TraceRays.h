#pragma once

#include <string>
#include <string.h>
#include <vector>
#include <memory>

#include "Visualization.h"
#include "Rays.h"
#include "Lighting.h"

using namespace std;

#include "ISPCObject.h"

namespace pvol
{
KEYED_OBJECT_POINTER(TraceRays)

#include "dtypes.h"

class TraceRays : public ISPCObject
{
public:
  TraceRays();
  ~TraceRays();

  virtual void LoadStateFromValue(Value&);
  virtual void SaveStateToValue(Value&, Document&);

  void SetEpsilon(float e);
  float GetEpsilon();

  RayList *Trace(Lighting*, VisualizationP, RayList *);

  virtual int SerialSize();
  virtual unsigned char *Serialize(unsigned char *);
  virtual unsigned char *Deserialize(unsigned char *);

protected:

  virtual void allocate_ispc();
  virtual void initialize_ispc();

};
}

