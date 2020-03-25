#include <mpi.h>

#include <stdio.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

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

int mpi_rank = 0, mpi_size;

void syntax(char *a)
{
  if (mpi_rank == 0)
    cerr << "syntax: " << " host port layout.json data.json\n";
  MPI_Finalize();
  exit(0);
}

int
main(int argc, char **argv)
{
#if 0
  std::cerr << getpid() << "\n";
  int dbg = 1;
  while(dbg)
    sleep(1);
#endif

  int port = 5001;
  std::string host = "localhost";
  std::string layout_file = "layout.json";
  std::string datafiles[2];
  datafiles[0] = "";
  datafiles[1] = "";

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  for (int i = 1; i < argc; i++)
    if (argv[i][0] == '-')
      switch(argv[i][1])
      {
        case 'l': layout_file = argv[++i]; break;
        case 'p': port = atoi(argv[++i]); break;
        case 'h': host = argv[++i]; break;
        default:
          syntax(argv[0]);
      }
    else if (datafiles[0] == "")
      datafiles[0] = argv[i];
    else if (datafiles[1] == "")
      datafiles[1] = argv[i];
    else 
      syntax(argv[0]);
    
  char *layout;
  int layout_sz;

  gxy::SocketHandler *master = NULL;
  int status = 1;

  if (mpi_rank == 0)
  {
    master = new gxy::SocketHandler();

    if (! master->Connect(host.c_str(), port))
    {
      std::cerr << "error: unable to connect to server (" << host << "," << port << ")\n";
      status = 0;
    }

    if (status)
    {
      std::string load = std::string("load libgxy_module_insitu.so");
      if (! master->CSendRecv(load))
      {
        std::cerr << "sending sofile failed\n";
        status = 0;
      }
    }

    if (status)
    {
      FILE *fp = fopen(layout_file.c_str(), "r");
      if (! fp)
      {
        std::cerr << "unable to read layout\n";
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
  if (mpi_rank != 0)
    layout = new char[layout_sz];

  MPI_Bcast(layout, layout_sz, MPI_CHAR, 0, MPI_COMM_WORLD);

  Document layout_doc;
  layout_doc.Parse(layout);

  Value& myConnectionInfo = layout_doc["layout"][mpi_rank];

  struct header
  {
    int nvars;
    int grid[3];    
    int pgrid[3];  
    int ghosts[6];
    int neighbors[6];
    int extent[6];
    double origin[3];
    double spacing[3];
  } hdr;

  std::vector<std::string> varnames;
  std::vector<float *> dataptrs[2];
  std::vector<float *> interpolated;
  std::vector<bool> isvector;
  int point_count;

  for (auto t = 0; t < 2; t++)
  {
    std::string datadesc_file = datafiles[t];

    int datadesc_sz;
    char *datadesc;

    // If master, load the file and distribute its contents

    if (mpi_rank == 0)
    {
      FILE *fp = fopen(datadesc_file.c_str(), "r");
      if (! fp)
      {
        std::cerr << "unable to read datadesc\n";
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

    if (mpi_rank == 0 && t == 0)
    {
      std::string cmd = std::string("new ") + datadesc + ";";
      if (! master->CSendRecv(cmd))
      {
        std::cerr << "sending new failed... " << cmd << "\n";
        status = 0;
      }
    }
  
    MPI_Bcast(&datadesc_sz, sizeof(datadesc_sz), MPI_CHAR, 0, MPI_COMM_WORLD);
    if (mpi_rank != 0)
      datadesc = new char[datadesc_sz];

    MPI_Bcast(datadesc, datadesc_sz, MPI_CHAR, 0, MPI_COMM_WORLD);

    // Parse the datadesc

    Document datadesc_doc;
    datadesc_doc.Parse(datadesc);

    // Where to find the data

    std::string datadir;
    if (t == 0)
    {
      size_t l = datadesc_file.find_last_of('/');
      if (l != std::string::npos)
        datadir = datadesc_file.substr(0, l+1);
      else
        datadir = "";
    }

    // Load local portion

    Value& myPartDoc = datadesc_doc["partitions"][mpi_rank];

    vtkXMLImageDataReader *rdr = vtkXMLImageDataReader::New();
    std::string fname = datadir + std::string(myPartDoc["file"].GetString());
    rdr->SetFileName(fname.c_str());
    rdr->Update();
    vtkImageData *vti = rdr->GetOutput();
  
    
    // Only set up the header for the first time step; assumption is both 
    // datasets are defined on the same grid

    if (t == 0)
    {
      for (int i = 0; i < 6; i++)
      {
        hdr.ghosts[i] = myPartDoc["ghosts"][i].GetInt();
        hdr.neighbors[i] = myPartDoc["neighbors"][i].GetInt();
      }

      hdr.grid[0] = (datadesc_doc["extent"][1].GetInt() - datadesc_doc["extent"][0].GetInt()) + 1;
      hdr.grid[1] = (datadesc_doc["extent"][3].GetInt() - datadesc_doc["extent"][2].GetInt()) + 1;
      hdr.grid[2] = (datadesc_doc["extent"][5].GetInt() - datadesc_doc["extent"][4].GetInt()) + 1;

      hdr.pgrid[0] = datadesc_doc["partition grid"][0].GetInt();
      hdr.pgrid[1] = datadesc_doc["partition grid"][1].GetInt();
      hdr.pgrid[2] = datadesc_doc["partition grid"][2].GetInt();

      vti->GetExtent(hdr.extent);
      vti->GetOrigin(hdr.origin); 
      vti->GetSpacing(hdr.spacing);

      hdr.nvars = datadesc_doc["variables"].Size();

      // How many local points?

      point_count = ((hdr.extent[1] - hdr.extent[0])+1)
                    * ((hdr.extent[3] - hdr.extent[2])+1)
                    * ((hdr.extent[5] - hdr.extent[4])+1);
    }

    // get pointer to data for each var.   If double, allocate and transform.  While doing
    // so, if this is t0, allocate interpolation buffer

    vtkPointData *pd = vti->GetPointData();

    Value variables = datadesc_doc["variables"].GetArray();
    for (Value::ConstValueIterator itr = variables.Begin(); itr != variables.End(); ++itr)
    {
      auto& v = *itr;

      std::string name = v["name"].GetString();

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
        dataptrs[t].push_back((float *)(a->GetVoidPointer(0)));
    }

    rdr->Delete();
  }

  // Now for each output time step:

  for (int it = 0; it < 10; it++)
  {
    // Tell Galaxy a timestep is on the way

    if (mpi_rank == 0)
    {
      std::string cmd = std::string("accept;");
      if (! master->CSendRecv(cmd))
      {
        std::cerr << "sending accept failed... " << cmd << "\n";
        status = 0;
      }
    }
  
    float t = it / 9.0;

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

    std::cerr << "attaching to " << host << ":" << port << "\n";

    if (0 == skt->ConnectToServer(host.c_str(), port))
    {
      skt->Send((const void *)&hdr, sizeof(hdr));

      for (int i = 0; i < varnames.size(); i++)
      {
        std::string name = varnames[i];

        int n = name.size() + 1;
        skt->Send(&n, sizeof(n));
        skt->Send((const void *)name.c_str(), n);

        n = isvector[i] ? 3 : 1;
        skt->Send(&n, sizeof(n));

        skt->Send(interpolated[i], point_count * (isvector[i] ? 3 : 1) * sizeof(float));
      }
    }
  }

  if (mpi_rank == 0)
    master->Disconnect();

  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
}
