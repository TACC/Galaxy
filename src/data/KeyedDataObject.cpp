#include "Application.h"
#include "KeyedDataObject.h"
#include "Datasets.h"
#include "Geometry.h"
#include "Volume.h"

namespace pvol
{

WORK_CLASS_TYPE(KeyedDataObject::ImportMsg);
WORK_CLASS_TYPE(KeyedDataObject::AttachMsg);
WORK_CLASS_TYPE(KeyedDataObject::LoadTimestepMsg);

void
KeyedDataObject::Register()
{
  RegisterClass();

  ImportMsg::Register();
  AttachMsg::Register();
  LoadTimestepMsg::Register();

  Datasets::Register();
  Geometry::Register();
  Volume::Register();
}

KEYED_OBJECT_TYPE(KeyedDataObject)

KeyedDataObject::~KeyedDataObject()
{
  //std::cerr << "~KeyedDataObject: " << this << " skt " << skt << "\n";
  if (skt)          
  {
    skt->CloseSocket();
    skt->Delete();
  }
}

void 
KeyedDataObject::CopyPartitioning(KeyedDataObjectP o)
{
	local_box = *o->get_local_box();
	global_box = *o->get_global_box();
	for (int i = 0; i < 6; i++)
			neighbors[i] = o->get_neighbor(i);
}

void 
KeyedDataObject::initialize()
{
	super::initialize();
  time_varying = false;
  attached = false;
  skt = NULL;
}

bool
KeyedDataObject::local_commit(MPI_Comm c)
{
  return false;
}

bool
KeyedDataObject::Attach(string layout) { return Attach(layout, NULL, 0); }

bool
KeyedDataObject::Attach(string layout, void *args, int argsSize)
{
  AttachMsg msg(sizeof(Key) + layout.length() + 1 + argsSize);
  char *p = (char *)msg.get();
  *(Key *)p = getkey();
  p = p + sizeof(Key);
  memcpy(p, layout.c_str(), layout.length());
  p = p + layout.length();
  *p++ = 0;

  if (argsSize > 0)
    memcpy(p, args, argsSize);

  msg.Broadcast(true, true);
  return skt != NULL;
}

void
KeyedDataObject::Import(string filename) { Import(filename, NULL, 0); }

void
KeyedDataObject::Import(string filename, void *args, int argsSize)
{
  ImportMsg msg(getkey(), filename, args, argsSize);
  msg.Broadcast(true, true);
}

bool
KeyedDataObject::WaitForTimestep()
{
  if (! skt)
    return false;

  int flag = -1;
  if ((skt->Receive((void *)&flag, sizeof(flag)) != sizeof(flag)) || (flag < 1))
  {
    skt->CloseSocket();
    skt->Delete();
    skt = NULL;
    return false;
  }
  else
    return true;
}

void
KeyedDataObject::LoadTimestep()
{
  LoadTimestepMsg msg(getkey());
  msg.Broadcast(true, true);
}

void
KeyedDataObject::local_import(char *s, MPI_Comm c)
{
  std::cerr << "generic KeyedDataObject::local_import called?\n";
  exit(0);
}

bool
KeyedDataObject::local_load_timestep(MPI_Comm c)
{
  std::cerr << "generic KeyedDataObject::local_load_timestep called?\n";
  exit(0);
}

bool
KeyedDataObject::AttachMsg::CollectiveAction(MPI_Comm c, bool isRoot)
{
  char *p = (char *)contents->get();
  Key k = *(Key *)p;
  p += sizeof(Key);

  KeyedDataObjectP o = KeyedDataObject::GetByKey(k);

  int rank = GetTheApplication()->GetRank();

  string server;
  int port;

  ifstream ifs(p);
  for (int i = 0; i <= rank; i++)
  {
    if (ifs.eof()) {cerr << "error getting attachment point\n"; exit(1); }
    ifs >> server >> port;
  }

  o->skt = vtkClientSocket::New();
  if (o->skt->ConnectToServer(server.c_str(), port))
  {
    o->skt->Delete();
    o->skt = NULL;
  }

  return false;
}

}
