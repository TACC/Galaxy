#include <iostream>
#include <vector>
#include <sstream>
using namespace std;

#include <string.h>
#include <pthread.h>

#include "Application.h"
#include "Datasets.h"
#include "MultiServer.h"
#include "MultiServerSocket.h"
using namespace gxy;

#include "rapidjson/filereadstream.h"
using namespace rapidjson;

extern "C" void
init()
{
  extern void InitializeData();
  InitializeData();
}

extern "C" bool
server(MultiServer *srvr, MultiServerSocket *skt)
{
  DatasetsP theDatasets = Datasets::Cast(GetTheApplication()->GetGlobal("global datasets"));
  if (! theDatasets)
  {
    theDatasets = Datasets::NewP();
    GetTheApplication()->SetGlobal("global datasets", theDatasets);
  }

  bool client_done = false;
  while (! client_done)
  {
    char *buf; int sz;
    if (! skt->CRecv(buf, sz))
      break;

    cerr << "received " << buf << "\n";

    if (!strncmp(buf, "import", strlen("import")))
    {
      Document doc;
      doc.Parse(buf+strlen("import"));

      theDatasets->LoadFromJSON(doc);
      std::string ok("ok");
      skt->CSend(ok.c_str(), ok.length()+1);
    }
    else if (!strncmp(buf, "list", strlen("list")))
    {
      if (! theDatasets)
      {
        std::string err("unable to access datasets");
        skt->CSend(err.c_str(), err.length()+1);
        continue;
      }

      vector<string> names = theDatasets->GetDatasetNames();

      std::string s("datasets:\n");
      for (vector<string>::iterator it = names.begin(); it != names.end(); ++it)
      {
        s.append(*it);
        s.append("\n");
      }

      skt->CSend(s.c_str(), s.length()+1);
    }
    else if (!strncmp(buf, "drop", strlen("drop")))
    {
      char objName[256], cmd[256];
      sscanf(buf, "%s%s", cmd, objName);
      theDatasets->DropDataset(objName);
      std::string ok("ok");
      skt->CSend(ok.c_str(), ok.length()+1);
    }
    else if (!strncmp(buf, "delete", strlen("delete")))
    {
      char objName[256], cmd[256];
      sscanf(buf, "%s%s", cmd, objName);
      KeyedDataObjectP kop = theDatasets->Find(objName);
      if (kop)
      {
        theDatasets->DropDataset(objName);
        Delete(kop);
      }
      else
        cerr << "couldn't find it in theDatasets\n";

      std::string ok("ok");
      skt->CSend(ok.c_str(), ok.length()+1);
    }
    else if (!strcmp(buf, "quit"))
    {
      client_done = true;
      std::string ok("quit ok");
      skt->CSend(ok.c_str(), ok.length()+1);
    }
    else
      cerr << "huh?\n";

    free(buf);
  }

  return true;
}
