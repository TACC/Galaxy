#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

using namespace std;

#include <Application.h>
#include <KeyedObject.h>

KEYED_OBJECT_POINTER(TestObject)

class TestObject : public KeyedObject
{
	KEYED_OBJECT(TestObject)

private:
	class DoitMsg : public Work
  {
  public:
    DoitMsg(Key k, string foo) : DoitMsg(sizeof(Key) + foo.length() + 1)
    {
      *(Key *)contents->get() = k;
      memcpy((char *)contents->get() + sizeof(Key), foo.c_str(), foo.length()+1);
    }

    WORK_CLASS(DoitMsg, true);

  public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {
			Key k = *(Key *)contents->get();
			char *foo = ((char *)contents->get()) + sizeof(k);
			APP_PRINT(<< ((int)k) << " " << foo);
      return false;
    }
  };

public:

	virtual void initialize();
	virtual ~TestObject();

	void doit()
	{
		DoitMsg m(getkey(), string("the rain in Spain"));
		m.Broadcast(true, true);
	}
};
