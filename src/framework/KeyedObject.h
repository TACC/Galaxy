#pragma once 

#include <string>
#include <iostream>
#include <memory>
#include <vector>

#include "rapidjson/document.h"
#include "Work.h"

namespace gxy
{
class KeyedObject;
typedef std::shared_ptr<KeyedObject> KeyedObjectP;

typedef int KeyedObjectClass;
typedef long Key;

class KeyedObjectFactory;
extern KeyedObjectFactory* GetTheKeyedObjectFactory();

class KeyedObject
{
public:
  KeyedObject(KeyedObjectClass c, Key k);
  virtual ~KeyedObject();

	static void Register()
	{
		CommitMsg::Register();
	};

	virtual void Drop();

  KeyedObjectClass getclass() { return keyedObjectClass; }
  Key getkey() { return key; }

  int SerialSize() { return serialSize(); }

  static KeyedObjectP Deserialize(unsigned char *p);
  unsigned char *Serialize(unsigned char *p);

  virtual void initialize() {}

  virtual int serialSize();
  virtual unsigned char *serialize(unsigned char *);
  virtual unsigned char *deserialize(unsigned char *);

  virtual void Commit();
  virtual bool local_commit(MPI_Comm);

	// only concrete subclasses have static LoadToJSON at abstract layer

  virtual void LoadFromJSON(rapidjson::Value&) { std::cerr << "abstract KeyedObject LoadFromJSON\n"; }
  virtual void SaveToJSON(rapidjson::Value&, rapidjson::Document&) { std::cerr << "abstract KeyedObject SaveToJSON\n"; }

  class CommitMsg : public Work
  {
  public:
    CommitMsg(KeyedObject* kop);

    WORK_CLASS(CommitMsg, false);

  public:
    bool CollectiveAction(MPI_Comm c, bool isRoot);
  };

  KeyedObjectClass keyedObjectClass;
  Key key;
};

class KeyedObjectFactory
{
public:
	KeyedObjectFactory() { next_key = 0; }
	~KeyedObjectFactory();
  int register_class(KeyedObject *(*n)(Key), std::string s)
  {
    new_procs.push_back(n);
    class_names.push_back(s);
    return new_procs.size() - 1;
  }

	static void Register()
	{
		NewMsg::Register();
		DropMsg::Register();
	}

  KeyedObjectP New(KeyedObjectClass c)
  {
    Key k = keygen();
    KeyedObjectP kop = std::shared_ptr<KeyedObject>(new_procs[c](k));
    add(kop);

    NewMsg msg(c, k);
    msg.Broadcast(true);

    return kop;
  }

  KeyedObjectP New(KeyedObjectClass c, Key k)
  {
    KeyedObjectP kop = std::shared_ptr<KeyedObject>(new_procs[c](k));
    add(kop);

    return kop;
  }

  std::string GetClassName(KeyedObjectClass c) 
  { 
    return class_names[c];
  }

  KeyedObjectP get(Key k);
  void add(KeyedObjectP p);

	void Dump();

	void Drop(Key k);

  void erase(Key k);

  Key keygen()
  {
    return (Key)next_key++;
  }

private:

  std::vector<KeyedObject*(*)(Key)> new_procs;
  std::vector<std::string> class_names;
  std::vector<KeyedObjectP> kmap;

	int next_key;

public:

  class NewMsg : public Work
  {
  public:

    NewMsg(KeyedObjectClass c, Key k) : NewMsg(sizeof(KeyedObjectClass) + sizeof(Key))
    {
      unsigned char *p = (unsigned char *)get();
      *(KeyedObjectClass *)p = c;
      p += sizeof(KeyedObjectClass);
      *(Key *)p = k;
    }

    WORK_CLASS(NewMsg, false);

  public:
    bool CollectiveAction(MPI_Comm comm, bool isRoot);
  };

  class DropMsg : public Work
  {
  public:

    DropMsg(Key k) : DropMsg(sizeof(Key))
    {
      unsigned char *p = (unsigned char *)get();
      *(Key *)p = k;
    }

    WORK_CLASS(DropMsg, false);

  public:
    bool CollectiveAction(MPI_Comm comm, bool isRoot);
  };
};

#define KEYED_OBJECT_POINTER(typ)                                                         \
class typ;                                                                                \
typedef std::shared_ptr<typ> typ ## P;                                                    \
typedef std::weak_ptr<typ> typ ## W;

#define KEYED_OBJECT_TYPE(typ)                                                            \
int typ::ClassType;                                                        

#define KEYED_OBJECT(typ)  KEYED_OBJECT_SUBCLASS(typ, KeyedObject)

#define KEYED_OBJECT_SUBCLASS(typ, parent)                                                \
                                                                                          \
protected:                                                                                \
  typ(Key k = -1) : parent(ClassType, k) {}                                               \
  typ(KeyedObjectClass c, Key k = -1) : parent(c, k) {}                                   \
                                                                                          \
private:                                                                                  \
  static KeyedObject *_New(Key k)                                                         \
  {                                                                                       \
    typ *t = new typ(k);                                                                  \
    t->initialize();                                                                      \
    return (KeyedObject *)t;                                                              \
  }                                                                                       \
                                                                                          \
public:                                                                                   \
  typedef parent super;                                                                   \
  KeyedObjectClass GetClass() { return keyedObjectClass; }                                \
  static bool IsA(KeyedObjectP a) { return dynamic_cast<typ *>(a.get()) != NULL; }        \
  static typ ## P Cast(KeyedObjectP kop) { return std::dynamic_pointer_cast<typ>(kop); }  \
  static typ ## P GetByKey(Key k) { return Cast(GetTheKeyedObjectFactory()->get(k)); }    \
  static void Register();                                                                 \
                                                                                          \
  std::string GetClassName()                                                              \
  {                                                                                       \
    return GetTheKeyedObjectFactory()->GetClassName(keyedObjectClass);                    \
  }                                                                                       \
                                                                                          \
  static typ ## P NewP()                                                                  \
  {                                                                                       \
    KeyedObjectP kop = GetTheKeyedObjectFactory()->New(ClassType);                        \
    return Cast(kop);                                                                     \
  }                                                                                       \
                                                                                          \
  static void RegisterClass()                                                             \
  {                                                                                       \
    typ::ClassType = GetTheKeyedObjectFactory()->register_class(typ::_New, std::string(#typ)); \
  }                                                                                       \
  static int  ClassType;

}