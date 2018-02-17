#define LOGGING 0

#include "Application.h"
#include "KeyedObject.h"

static int ko_count = 0;
static int get_ko_count() { return ko_count; }

WORK_CLASS_TYPE(KeyedObject::CommitMsg)
WORK_CLASS_TYPE(KeyedObjectFactory::NewMsg)
WORK_CLASS_TYPE(KeyedObjectFactory::DropMsg)

KeyedObjectFactory* GetTheKeyedObjectFactory()
{
	return GetTheApplication()->GetTheKeyedObjectFactory();
}

bool
KeyedObjectFactory::NewMsg::CollectiveAction(MPI_Comm comm, bool isRoot)
{
	if (!isRoot)
	{
		unsigned char *p = (unsigned char *)get();
		KeyedObjectClass c = *(KeyedObjectClass *)p;
		p += sizeof(KeyedObjectClass);
		Key k = *(Key *)p;
		KeyedObjectP kop = shared_ptr<KeyedObject>(GetTheKeyedObjectFactory()->New(c, k));
	}

	return false;
}

void
KeyedObjectFactory::Drop(Key k)
{
#if LOGGING
	APP_LOG(<< "DROP " << k);
#endif
	DropMsg msg(k);
	msg.Broadcast(true);
}

bool 
KeyedObjectFactory::DropMsg::CollectiveAction(MPI_Comm comm, bool isRoot)
{
	unsigned char *p = (unsigned char *)get();
	Key k = *(Key *)p;
	GetTheKeyedObjectFactory()->erase(k);
	return false;
}

KeyedObjectP
KeyedObjectFactory::get(Key k)
{
	if (k >= kmap.size())
		return NULL;
	else
		return kmap[k];
}

void
KeyedObjectFactory::erase(Key k)
{
	kmap[k] = NULL;
}

void
KeyedObjectFactory::add(KeyedObjectP p)
{
#if 0
	for (int i = kmap.size(); i < p->getkey(); i++)
		kmap.push_back(NULL);

	kmap.push_back(p);
#else
  	for (int i = kmap.size(); i <= p->getkey(); i++)
		kmap.push_back(NULL);

	kmap[p->getkey()] = p;
#endif
}

void
KeyedObject::Drop()
{
	GetTheKeyedObjectFactory()->Drop(getkey());
}

KeyedObject::KeyedObject(KeyedObjectClass c, Key k) : keyedObjectClass(c), key(k)
{  
	ko_count++;
	// std::cerr << "KeyedObject ctor: " << GetTheKeyedObjectFactory()->GetClassName(c) << " key " << k << "\n";
	initialize();
}

KeyedObject::~KeyedObject()
{  
  
	ko_count--;
}

bool
KeyedObject::local_commit(MPI_Comm c)
{
	return false;
}

KeyedObjectP
KeyedObject::Deserialize(unsigned char *p)
{
	Key k = *(Key *)p;
	p += sizeof(Key);

	KeyedObjectP kop = GetTheKeyedObjectFactory()->get(k);
	p = kop->deserialize(p);

	if (*(int *)p != 12345)
	{
		std::cerr << "Serialization error!\n";
		exit(1);
	}

	return kop;
}

unsigned char *
KeyedObject::Serialize(unsigned char *p)
{
	*(Key *)p = getkey();
	p += sizeof(Key);
	p = serialize(p);
	*(int*)p = 12345;
	p += sizeof(int);
	return p;
}

int
KeyedObject::serialSize()
{
	return sizeof(Key) + sizeof(int);
}

unsigned char *
KeyedObject::serialize(unsigned char *p)
{
	return p;
}

unsigned char *
KeyedObject::deserialize(unsigned char *p)
{
	return p;
}

void
KeyedObject::Commit()
{
	CommitMsg msg(this);
	msg.Broadcast(true);
}

KeyedObject::CommitMsg::CommitMsg(KeyedObject* kop) : KeyedObject::CommitMsg::CommitMsg(kop->SerialSize())
{
	unsigned char *p = (unsigned char *)get();
	kop->Serialize(p);
}

bool
KeyedObject::CommitMsg::CollectiveAction(MPI_Comm c, bool isRoot)
{
  unsigned char *p = (unsigned char *)get();
  Key k = *(Key *)p;
  p += sizeof(Key);

  KeyedObjectP kop = GetTheKeyedObjectFactory()->get(k);

  if (! isRoot)
    kop->deserialize(p);

  return kop->local_commit(c);
}

void
KeyedObjectFactory::Dump()
{
	for (int i = 0; i < kmap.size(); i++)
	{
		KeyedObjectP kop = kmap[i];
		if (kop != NULL)
			std::cerr << "key " << i << " " << GetClassName(kop->getclass()) << " count " << kop.use_count() << "\n";
	}
}

KeyedObjectFactory::~KeyedObjectFactory() 
{
	while (kmap.size() > 0)
		kmap.pop_back();

	if (ko_count > 0)
		std::cerr << "shared objects remain\n";
}
