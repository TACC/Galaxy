#pragma once

#include <string>
#include <string.h>
#include <vector>
#include <memory>

using namespace std;

#include "KeyedObject.h"
#include "ISPCObject.h"


#include "dtypes.h"
#include "OSPRayObject.h"
#include "Datasets.h"

namespace pvol
{

KEYED_OBJECT_POINTER(Vis)
  
class Vis : public KeyedObject, public ISPCObject
{
    KEYED_OBJECT(Vis)

public:
    virtual ~Vis();

    virtual void Commit(DatasetsP);
    OSPRayObjectP GetTheData() { return data; }

    virtual void LoadFromJSON(Value&);
    virtual void SaveToJSON(Value&, Document&);
    void SetName(string n) { name = n; }
    virtual bool local_commit(MPI_Comm);

protected:
    virtual void allocate_ispc();
    virtual void initialize_ispc();
    virtual void destroy_ispc();

    virtual int serialSize();
    virtual unsigned char *serialize(unsigned char *);
    virtual unsigned char *deserialize(unsigned char *);

    string name;
    Key datakey;
    OSPRayObjectP data;
};

}
