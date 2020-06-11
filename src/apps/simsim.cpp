// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
// All rights reserved.                                                       //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// A copy of the License is included with this software in the file LICENSE.  //
// If your copy does not contain the License, you may obtain a copy of the    //
// License at:                                                                //
//                                                                            //
//     https://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  //
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
//                                                                            //
// ========================================================================== //

#include <mpi.h>

#include <stdio.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

using namespace rapidjson;

#include <vtkClientSocket.h>
#include <vtkImageData.h>
#include <vtkXMLImageDataReader.h>
#include <vtkDataSetWriter.h>
#include <vtkPointData.h>
#include <vtkAbstractArray.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>

#include "SocketHandler.h"

int mpiRank = 0, mpiSize;

#include "Debug.h"


void syntax(char *a)
{
  if (mpiRank == 0)
  {
    cerr << "syntax: " << " [options] data.json\n";
    cerr << "options:\n";
    cerr << "  -h host          host name (localhost)\n";
    cerr << "  -d delay         wait delay sec between timesteps (5)\n";
    cerr << "  -n nsteps        number of timesteps (10)\n";
    cerr << "  -p port          port (5001)\n";
    cerr << "  -l layout        layout file (layout.json)\n";
    cerr << "  -c               cycle\n";
  }
    
  MPI_Finalize();
  exit(0);
}

#include <signal.h>

static gxy::SocketHandler *master = NULL;

void
IntHandler(int s)
{
  std::cerr << "interrupt\n";
  if (master)
  {
    string cmd = string("close;");
    if (! master->CSendRecv(cmd))
      cerr << "sending close failed... " << cmd << "\n";
 
    master->Disconnect();
  }
  exit(0);
}

struct header
{
  size_t datasize;
  int extent[6];
  double origin[3];
  double spacing[3];
};

void
SerializeVTI(vtkSocket *skt, vtkImageData* vti, std::vector<std::string> vars)
{
  struct header hdr;

  vti->GetExtent(hdr.extent);
  vti->GetOrigin(hdr.origin);
  vti->GetSpacing(hdr.spacing);

  int point_count = ((hdr.extent[1] - hdr.extent[0])+1) 
                  * ((hdr.extent[3] - hdr.extent[2])+1)
                  * ((hdr.extent[5] - hdr.extent[4])+1);

  vtkPointData *pd = vti->GetPointData();

  hdr.datasize = 0;
  for (auto v : vars)
  {
    vtkAbstractArray *a = pd->GetArray(v.c_str());

    if (! vtkFloatArray::SafeDownCast(a))
    {
      std::cerr << "can't handle " << v << "... not float\n";
      exit(0);
    }

    int nc = a->GetNumberOfComponents();

    if (nc != 1 && nc != 3)
    {
      std::cerr << "can't handle " << v << "... gotta be scalar or 3-vector\n";
      exit(0);
    }

    hdr.datasize += v.size() + 1;
    hdr.datasize += point_count * nc * sizeof(float);
  }

  skt->Send((const void *)&hdr, sizeof(hdr));

  for (auto v : vars)
  {
    vtkAbstractArray *a = pd->GetArray(v.c_str());
    int nc = a->GetNumberOfComponents();
    skt->Send((const void *)v.c_str(), v.size()+1);
    skt->Send(a->GetVoidPointer(0), point_count * nc * sizeof(float));
  }
}

int
main(int argc, char **argv)
{
  bool dbg = false, atch = false;
  char *dbgarg = NULL;
  int delay = 5;
  int nsteps = 10;
  bool cycle = false;

  signal(SIGINT, IntHandler);

  int port = 5001;
  string host = "localhost";
  string layout_file = "layout.json";
  string datafiles[2];
  datafiles[0] = "";
  datafiles[1] = "";

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

  for (int i = 1; i < argc; i++)
    if (argv[i][0] == '-')
      switch(argv[i][1])
      {
        case 'l': layout_file = argv[++i]; break;
        case 'p': port = atoi(argv[++i]); break;
        case 'h': host = argv[++i]; break;
        case 'D': dbg = true; dbgarg = argv[i] + 2; break;
        case 'd': delay = atoi(argv[++i]); break;
        case 'n': nsteps = atoi(argv[++i]); break;
        case 'c': cycle = true; break;
        default:
          syntax(argv[0]);
      }
    else if (datafiles[0] == "")
      datafiles[0] = argv[i];
    else if (datafiles[1] == "")
      datafiles[1] = argv[i];
    else 
      syntax(argv[0]);

  if (delay < 0) delay = 0;
  else if (delay > 100) delay = 100;

  if (nsteps < 1) nsteps = 1;
  else if (nsteps > 100) nsteps = 100;

  Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;
    
  char *layout;
  int layout_sz;

  int status = 1;

  if (mpiRank == 0)
  {
    master = new gxy::SocketHandler();

    if (! master->Connect(host.c_str(), port))
    {
      cerr << "error: unable to connect to server (" << host << "," << port << ")\n";
      status = 0;
    }

    if (status)
    {
      string load = string("load libgxy_module_insitu.so");
      if (! master->CSendRecv(load))
      {
        cerr << "sending sofile failed\n";
        status = 0;
      }
    }

    if (status)
    {
      FILE *fp = fopen(layout_file.c_str(), "r");
      if (! fp)
      {
        cerr << "unable to read layout\n";
        status = 0;
      }
      else
      {
        fseek(fp, 0, SEEK_END);
        layout_sz = (size_t)ftell(fp) + 1;
        fseek(fp, 0, SEEK_SET);

        layout = new char[layout_sz];
        for (int s = 0; s < (layout_sz-1); s += fread(layout + s, 1, (layout_sz-1) - s, fp));
        fclose(fp);

        layout[layout_sz-1] = '\0';
      }
    }
  }

  MPI_Bcast(&status, sizeof(status), MPI_CHAR, 0, MPI_COMM_WORLD);
  if (!status)
  {
    MPI_Finalize();
    exit(1);
  }

  MPI_Bcast(&layout_sz, sizeof(layout_sz), MPI_CHAR, 0, MPI_COMM_WORLD);
  if (mpiRank != 0)
    layout = new char[layout_sz];

  MPI_Bcast(layout, layout_sz, MPI_CHAR, 0, MPI_COMM_WORLD);

  Document layout_doc;
  layout_doc.Parse(layout);

  Value& myConnectionInfo = layout_doc["layout"][mpiRank];

  vector<string> varnames;
  vector<float *> dataptrs[2];
  vector<float *> interpolated;
  vector<bool> isvector;
  int point_count;

  for (auto t = 0; t < 2; t++)
  {
    string datadesc_file = datafiles[t];
    std::cerr << "loading " << datadesc_file << "\n";

    int datadesc_sz;
    char *datadesc;

    // If master, load the file and distribute its contents

    if (mpiRank == 0)
    {
      FILE *fp = fopen(datadesc_file.c_str(), "r");
      if (! fp)
      {
        cerr << "unable to read datadesc\n";
        status = 0;
      }
      else
      {
        fseek(fp, 0, SEEK_END);
        datadesc_sz = (size_t)ftell(fp) + 1;
        fseek(fp, 0, SEEK_SET);

        datadesc = new char[datadesc_sz];

        for (int s = 0; s < (datadesc_sz-1); s += fread(datadesc + s, 1, (datadesc_sz-1) - s, fp));
        fclose(fp);

        datadesc[datadesc_sz-1] = '\0';
      }
    }

    // If this is the first timestep, tell Galaxy what to expect

    if (mpiRank == 0 && t == 0)
    {
      string cmd = string("new ") + datadesc + ";";
      if (! master->CSendRecv(cmd))
      {
        cerr << "sending new failed... " << cmd << "\n";
        status = 0;
      }
    }
  
    MPI_Bcast(&datadesc_sz, sizeof(datadesc_sz), MPI_CHAR, 0, MPI_COMM_WORLD);
    if (mpiRank != 0)
      datadesc = new char[datadesc_sz];

    MPI_Bcast(datadesc, datadesc_sz, MPI_CHAR, 0, MPI_COMM_WORLD);

    // Parse the datadesc

    Document datadesc_doc;
    datadesc_doc.Parse(datadesc);

    // Where to find the data

    string datadir;
    if (t == 0)
    {
      size_t l = datadesc_file.find_last_of('/');
      if (l != string::npos)
        datadir = datadesc_file.substr(0, l+1);
      else
        datadir = "";
    }

    // Load local portion

    Value& myPartDoc = datadesc_doc["partitions"][mpiRank];

    vtkXMLImageDataReader *rdr = vtkXMLImageDataReader::New();
    string fname = datadir + string(myPartDoc["file"].GetString());
    rdr->SetFileName(fname.c_str());
    rdr->Update();
    vtkImageData *vti = rdr->GetOutput();

    // Only set up the header for the first time step; assumption is both 
    // datasets are defined on the same grid

    if (t == 0)
    {
      int extent[6];
      vti->GetExtent(extent);

      point_count = ((extent[1] - extent[0])+1) * ((extent[3] - extent[2])+1) * ((extent[5] - extent[4])+1);
    }

    // get pointer to data for each var.   If double, allocate and transform.  While doing
    // so, if this is t0, allocate interpolation buffer

    vtkPointData *pd = vti->GetPointData();

    Value variables = datadesc_doc["variables"].GetArray();
    for (Value::ConstValueIterator itr = variables.Begin(); itr != variables.End(); ++itr)
    {
      auto& v = *itr;

      string name = v["name"].GetString();

      // Def. need this on t=0 for interplants, may need it elsetimes if double

      int bufsz = point_count * (v["vector"].GetBool() ? 3 : 1) * sizeof(float);

      if (t == 0)
      {
        interpolated.push_back((float *)malloc(bufsz));
        isvector.push_back(v["vector"].GetBool());
        varnames.push_back(name);
      }

      vtkAbstractArray *a = pd->GetArray(name.c_str());
      if (vtkDoubleArray::SafeDownCast(a))
      {
        float *buf = (float *)malloc(point_count * (v["vector"].GetBool() ? 3 : 1) * sizeof(float));
        double *src = (double *)a->GetVoidPointer(0);
        float *dst = buf;
        for (int i = 0; i < point_count * (v["vector"].GetBool() ? 3 : 1); i++)
          *dst++ = float(*src++);
        dataptrs[t].push_back(buf);
      }
      else
      {
        void *buf = malloc(point_count * (v["vector"].GetBool() ? 3 : 1) * sizeof(float));
        void *src = a->GetVoidPointer(0);
        memcpy(buf, src, point_count * (v["vector"].GetBool() ? 3 : 1) * sizeof(float));
        dataptrs[t].push_back((float *)buf);
      }
    }

    rdr->Delete();
  }

  // Now for each output time step:

  int it = 0; bool done = false; int dir = 1;
  while (! done)
  {
    float t =  (nsteps == 1) ? 0.5 : it / float(nsteps - 1);

    for (int i = 0; i < varnames.size(); i++)
    {
      float *p0 = dataptrs[0][i];
      float *p1 = dataptrs[1][i];
      float *dst = interpolated[i];
      int count = point_count * (isvector[i] ? 3 : 1);

      for (int j = 0; j < count; j++)
      {
        *dst++ = *p0 + t*(*p1 - *p0);
        p0++; p1++;
      }
    }
      
    vtkClientSocket *skt = vtkClientSocket::New();

    host = myConnectionInfo["host"].GetString();
    port = myConnectionInfo["port"].GetInt();

    // Tell Galaxy a timestep is on the way

    for (int i = 0; i < varnames.size(); i++)
    {
      string name = varnames[i];

      if (mpiRank == 0)
      {
        string cmd = string("accept ") + name + ";";
        if (! master->CSend(cmd))
        {
          cerr << "sending accept failed... " << cmd << "\n";
          status = 0;
        }
      }

      MPI_Barrier(MPI_COMM_WORLD);
  
      skt->ConnectToServer(host.c_str(), port);
      skt->Send(interpolated[i], point_count * (isvector[i] ? 3 : 1) * sizeof(float));

      if (mpiRank == 0)
      {
        string rply;
        if (! master->CRecv(rply) || rply != "ok")
        {
          cerr << "accept failed... " << rply << "\n";
          status = 0;
        }
      }
    }

#if 0
    if (mpiRank == 0)
    {
      string cmd = string("commit;");
      if (! master->CSendRecv(cmd))
      {
        cerr << "sending commit failed... " << cmd << "\n";
        status = 0;
      }
    }
#endif

    skt->CloseSocket();
    skt->Delete();
    
    if (delay > 0)
    {
      std::cerr << "sleeping ";
      for (int i = delay; i > 0; i--)
      {
        if (mpiRank == 0)
          std::cerr << i << "...";
        sleep(1);
      }
      std::cerr << "\n";
    }

    if (nsteps == 0) done = true;
    else
    {
      if (dir == 1)
      {
        if (it == (nsteps - 1))
        {
          if (cycle == false)
            done = true;
          else
          {
            dir = -1;
            it = it - 1;
          }
        }
        else
          it = it + 1;
      }
      else
      {
        if (it == 0)
        {
          dir = 1;
          it = 1;
        }
        else
          it = it - 1;
      }
    }
  }

  if (mpiRank == 0)
  {
    string cmd = string("close;");
    if (! master->CSendRecv(cmd))
    {
      cerr << "sending close failed... " << cmd << "\n";
      status = 0;
    }

    master->Disconnect();
  }

  MPI_Finalize();
  exit(0);
}
