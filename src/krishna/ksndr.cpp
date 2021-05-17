// ========================================================================== //
//                                                                            //
// Copyright (c) 2014-2020 The University of Texas at Austin.                 //
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

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <climits>
#include <memory>

using namespace std;

#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPolyData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPoints.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>

#include <unistd.h>
#include <stdio.h> 
#include <stdlib.h>
#include <time.h> 
#include <pthread.h> 

#include "Skt.hpp"
#include "bufhdr.h"

using namespace gxy;

float gxmin = -1, gxmax = 1, gymin = -1, gymax = 1, gzmin = -1, gzmax = 1, gdmin, gdmax;
float fuzz = 0.0;

int generated_nPts = 100;
int n_senders = 1;
int running_count;
bool testdata = false;
string varname = "data";

int mpi_rank = 0, mpi_size = 1;

void
syntax(char *a)
{
  std::cerr << "syntax: " << a << " [options]\n";
  std::cerr << "options:\n";
  std::cerr << " -M master                    address of master process (localhost)\n";
  std::cerr << " -h hostfile                  a file containing a host name for each receiver process (localhost)\n";
  std::cerr << " Relevant when sending pre-computed data:\n";
  std::cerr << " -F pfile                     a file containing a list of constituent files (none)\n";
  std::cerr << " -V varname                   name of point-dependendent data to be used in the case of CTK file input (data)\n";
  std::cerr << " Relevant when generating random data:\n";
  std::cerr << " -n nproc                     number of senders to run (1)\n";
  std::cerr << " -p npart                     number of particles to send to each receiver (100)\n";
  std::cerr << " -f fuzz                      max radius of points - creates empty border region (0)\n";
  std::cerr << " -b box                       a file containing six space separated values: xmin xmax ymin ymax zmin zmax (-1 1 -1 1 -1 1)\n";
  std::cerr << " -B xl, yl, zl, xu, yu, zu    bounding box (default -1 -1 -1 1 1 1))\n";
  std::cerr << " -T                           test case: one point at the center\n";
  
  exit(1);
}

enum
{
  CREATE_SOME_DATA, SEND_SOME_DATA
} command;

class SenderThread
{
  static void *sender_thread(void *d)
  {
    SenderThread *me = (SenderThread *)d;

    if (command == SEND_SOME_DATA)
      me->SendSomeData();
    else 
      me->CreateSomeData();

    pthread_exit(NULL);
    return NULL;
  }
    
public:
  SenderThread(int n, string pfile = "") : sender_id(n), pfile(pfile)
  {
    long seed = getpid() + rand();
    mt_rand.seed(seed);
  }

  ~SenderThread()
  {
    if (tid != NULL)
    {
      pthread_join(tid, NULL);
      tid = NULL;
    }
  }

  void SetDestination(string h, int p)
  {
    host = h;
    port = p;
  }

  void
  Run()
  {
    pthread_create(&tid, NULL, sender_thread, (void *)this);
  }

  void
  Wait()
  {
    pthread_join(tid, NULL);
    tid = NULL;
  }

  void SetFuzz(float f)
  {
    fuzz = f;
  }

  void GetFuzz(float& f)
  {
    f = fuzz;
  }

  void
  GetBox(float& x, float& y, float& z, float& X, float& Y, float& Z)
  {
    x = xmin;
    X = xmax;
    y = ymin;
    Y = ymax;
    z = zmin;
    Z = zmax;
  }

  void 
  SetBox(float *b)
  {
    SetBox(b[0], b[1], b[2], b[3], b[4], b[5]);
  }

  void
  SetBox(float x, float y, float z, float X, float Y, float Z)
  {
    xmin = x;
    xmax = X;
    ymin = y;
    ymax = Y;
    zmin = z;
    zmax = Z;
  }

  void GetDataRange(float& d, float &D)
  {
    d = dmin;
    D = dmax;
  }

private:
  void
  CreateSomeData()
  {
    bufhdr *hdr;

    if (testdata)
    {
      nPts = 1;
      if (dsz == -1)
      {
        dsz = sizeof(int) + sizeof(bufhdr)+ nPts*4*sizeof(float);
        data = (char *)malloc(dsz);
      }

      *(int *)data = dsz;

      hdr = (bufhdr *)(data + sizeof(int));
      float *ptr =   (float *)(data + sizeof(int) + sizeof(bufhdr));
      
      *ptr++ = (xmin + xmax) / 2.0;
      *ptr++ = (ymin + ymax) / 2.0;
      *ptr++ = (zmin + zmax) / 2.0;
      *ptr++ = 0.0;
    }
    else if (pfile != "")
    {
      string ext = pfile.substr(pfile.size() - 3);
      if (ext == "vtp" || ext == "vtu")
      {
        vtkPointSet *ps;

        if (ext == "vtp")
        {
          vtkXMLPolyDataReader *rdr = vtkXMLPolyDataReader::New();
          rdr->SetFileName(pfile.c_str());
          rdr->Update();
          ps = (vtkPointSet *)rdr->GetOutput();
        }
        else
        {
          vtkXMLUnstructuredGridReader *rdr = vtkXMLUnstructuredGridReader::New();
          rdr->SetFileName(pfile.c_str());
          rdr->Update();
          ps = (vtkPointSet *)rdr->GetOutput();
        }

        vtkPoints *vpts = ps->GetPoints();
        nPts = ps->GetNumberOfPoints();

        dsz = sizeof(int) + sizeof(bufhdr)+ nPts*4*sizeof(float);
        data = (char *)malloc(dsz);
        *(int *)data = dsz;

        hdr = (bufhdr *)(data + sizeof(int));
        float *pdst = (float *)(hdr + 1);
        float *ddst = pdst + 3*nPts;

        vtkFloatArray *pArray = vtkFloatArray::SafeDownCast(vpts->GetData());
        if (pArray)
          memcpy(pdst, pArray->GetVoidPointer(0), nPts * 3 * sizeof(float));
        else
        {
          vtkDoubleArray *pArray = vtkDoubleArray::SafeDownCast(vpts->GetData());
          if (! pArray)
          {
            std::cerr << "points have to be float or double\n";
            exit(1);
          }

          double *psrc = (double *)pArray->GetVoidPointer(0);
          for (int i = 0; i < 3*nPts; i++)
            *pdst++ = (float)*psrc++;
        }

        vtkFloatArray *fArray = vtkFloatArray::SafeDownCast(ps->GetPointData()->GetArray(varname.c_str()));
        if (fArray)
        {
          float *fsrc = (float *)fArray->GetVoidPointer(0);
    
          if (fArray->GetNumberOfComponents() == 1)
          {
            memcpy(ddst, fArray->GetVoidPointer(0), nPts * sizeof(float));
          }
          else if (fArray->GetNumberOfComponents() == 3)
          {
            for (int i = 0; i < nPts; i++)
            {
              double a = fsrc[0]*fsrc[0] + fsrc[1]*fsrc[1] + fsrc[2]*fsrc[2];
              *ddst++ = sqrt(a);
              fsrc += 3;
            }
          }
          else
          {
            std::cerr << "data have to be scalar or 3-vector\n";
            exit(1);
          }
        }
        else
        {
          vtkDoubleArray *dArray = vtkDoubleArray::SafeDownCast(ps->GetPointData()->GetArray("displacements"));
          if (! dArray)
          {
            std::cerr << "data have to be float or double\n";
            exit(1);
          }

          double *dsrc = (double *)dArray->GetVoidPointer(0);

          if (dArray->GetNumberOfComponents() == 1)
          {
            for (int i = 0; i < nPts; i++)
              *ddst++ = *dsrc++;
          }
          else if (dArray->GetNumberOfComponents() == 3)
          {
            for (int i = 0; i < nPts; i++)
            {
              double a = dsrc[0]*dsrc[0] + dsrc[1]*dsrc[1] + dsrc[2]*dsrc[2];
              *ddst++ = sqrt(a);
              dsrc += 3;
            }
          }
          else
          {
            std::cerr << "data have to be scalar or 3-vector\n";
            exit(1);
          }
        }
      }
      else if (ext == "csv")
      {
        FILE *f = fopen(pfile.c_str(), "r");
        if (! f)
        {
          std::cerr << "unable to open " << pfile << "\n";
          exit(1);
        }

        fscanf(f, "%d", &nPts);

        dsz = sizeof(int) + sizeof(bufhdr)+ nPts*4*sizeof(float);
        data = (char *)malloc(dsz);
  
        hdr = (bufhdr *)(data + sizeof(int));
        float *pptr = (float *)(data + sizeof(int) + sizeof(bufhdr));
        float *dptr = pptr + 3*nPts;

        for (int i = 0; i < nPts; i++)
        {
          fscanf(f, "%f,%f,%f,%f", pptr, pptr+1, pptr+2, dptr);
          pptr += 3;
          dptr += 1;
        }
      }
      else if (ext == "raw")
      {
        std::fstream fs;
        fs.open(pfile, std::ios::in|std::ios::binary);
        if (! fs.good())
        {
          std::cerr << "bad\n";
          exit(1);
        }

        fs.read((char *)&nPts, sizeof(int));

        dsz = sizeof(int) + sizeof(bufhdr)+ nPts*4*sizeof(float);
        data = (char *)malloc(dsz);
  
        hdr = (bufhdr *)(data + sizeof(int));
        char *pptr = (char *)(data + sizeof(int) + sizeof(bufhdr));
        char *dptr = pptr + 3*nPts*sizeof(float);

        fs.read(pptr, 3*nPts*sizeof(float));
        fs.read(dptr, nPts*sizeof(float));
      }
      else
      {
        std::cerr << "pfile must be vtu, vtp, csv or raw\n";
        exit(1);
      }
    }
    else
    {
      srand(mpi_rank * 9719 + sender_id * 8513);

      if (nPts != generated_nPts)
      {
        if (data) free(data);
        nPts = generated_nPts;
        dsz = sizeof(int) + sizeof(bufhdr)+ nPts*4*sizeof(float);
        data = (char *)malloc(dsz);
      }

      nPts = generated_nPts;
      *(int *)data = dsz;

      hdr = (bufhdr *)(data + sizeof(int));
      float *ptr =   (float *)(data + sizeof(int) + sizeof(bufhdr));

      std::cerr << sender_id << ": " << xmin << " " << ymin << " " << zmin << " " << xmax << " " << ymax << " " << zmax << "\n";

      float fxmin = xmin + fuzz;
      float fymin = ymin + fuzz;
      float fzmin = zmin + fuzz;

      float fxmax = xmax - fuzz;
      float fymax = ymax - fuzz;
      float fzmax = zmax - fuzz;

      for (int i = 0; i < nPts; i++)
      {
        *ptr++ = fxmin + frand()*(fxmax - fxmin);
        *ptr++ = fymin + frand()*(fymax - fymin);
        *ptr++ = fzmin + frand()*(fzmax - fzmin);
      }

      for (int i = 0; i < nPts; i++)
        *ptr++ = n_senders > 1 ? float(sender_id) / (n_senders - 1) : 0.0;
    }

    hdr->type          = bufhdr::Particles;
    hdr->origin        = sender_id;
    hdr->has_data      = true;
    hdr->npoints       = nPts;
    hdr->nconnectivity = 0;

    if (! testdata)
    {
      float *pptr = (float *)(data + sizeof(int) + sizeof(bufhdr));
      float *dptr = pptr + 3*nPts;

      xmin = xmax = pptr[0];
      ymin = ymax = pptr[1];
      zmin = zmax = pptr[2];
      dmin = dmax = *dptr;

      for (int i = 0; i < nPts; i++)
      {
        if (xmin > pptr[0]) xmin = pptr[0];
        if (xmax < pptr[0]) xmax = pptr[0];
        if (ymin > pptr[1]) ymin = pptr[1];
        if (ymax < pptr[1]) ymax = pptr[1];
        if (zmin > pptr[2]) zmin = pptr[2];
        if (zmax < pptr[2]) zmax = pptr[2];
        if (dmin > *dptr) dmin = *dptr;
        if (dmax < *dptr) dmax = *dptr;
        pptr += 3;
        dptr ++;
      }
    }
  }

  bool
  SendSomeData()
  {
    if (port == -1)
    {
      std::cerr << "Can't send data without knowing where!\n";
      return false;
    }
    else
    {
      ClientSkt c(host, port);
      return c.Connect() && c.Send(data, dsz);
    }
  }

  float
  frand()
  {
    return float(mt_rand()) / UINT_MAX;
  }

private:
  string pfile;
  mt19937 mt_rand;
  pthread_t tid;
  int sender_id;
  size_t dsz = -1;
  char *data = NULL;
  int frame = 0;
  string host = "";
  int port = -1;
  int nPts = -1;
  float xmin = -1, xmax = 1, ymin = -1, ymax = 1, zmin = -1, zmax = 1, dmin = 0, dmax = 1;
};

vector< shared_ptr<SenderThread> > senders;

int 
Setup(ClientSkt* masterskt)
{
  int status = 0;
  int sz;
  char *hoststring = NULL;

  if (masterskt && masterskt->Connect())
  {
    status = 1;

    if (status && !masterskt->Send(std::string("box")))
      status = 0;

    float box[] = {gxmin, gymin, gzmin, gxmax, gymax, gzmax};
    if (status && !masterskt->Send((void *)box, 6*sizeof(float)))
      status = 0;

    std::stringstream ss;
    ss << "nsenders " << n_senders;
    if (status && !masterskt->Send(ss.str()))
      status = 0;

    if (status && !masterskt->Send(std::string("sendhosts")))
      status = 0;
  
    if (status && ((hoststring = masterskt->Receive()) == NULL))
      status = 0;

    if (status && !masterskt->Send(std::string("go")))
      status = 0;

    masterskt->Disconnect();
  }

  sz = status ? strlen(hoststring) : -1;

  MPI_Bcast(&status, 1, MPI_INT, 0, MPI_COMM_WORLD);

  if (status)
  {
    if (mpi_rank > 0)
      hoststring = (char *)malloc(sz);

    MPI_Bcast(&hoststring, sz, MPI_CHAR, 0, MPI_COMM_WORLD);

    // create a list of space-separated 'host port' destination strings
    // from a colon-separated of 'host:port's.

    std::vector<char *> hosts;
    hosts.push_back(hoststring);

    for (char *c = hoststring; *c; c++)
      if (*c == ':') *c = ' ';
      else if (*c == ';')
      {
        *c = '\0';
        if (*(c+1) != '\0')
          hosts.push_back(c+1);
      }

    int senders_per_rank = n_senders / mpi_size;
    int sender_start = mpi_rank * senders_per_rank;
    int sender_end   = (mpi_rank == mpi_size-1) ? n_senders : (mpi_rank+1) * senders_per_rank;

    for (int i = 0;  i < (sender_end - sender_start); i++)
    {
      int j = (i + sender_start) % hosts.size();

      char host[256]; int port;

      stringstream ss(hosts[j]);
      ss >> host >> port;

      senders[i]->SetDestination(host, port);
    }

  }

  if (hoststring)
    free(hoststring);

  return status;
}

bool
SendSomeData(ClientSkt* masterskt)
{
  int status = Setup(masterskt);

  if (status)
  {
    command = SEND_SOME_DATA;

    for (auto s : senders)
      s->Run();

    for (auto s : senders)
      s->Wait();
  }

  return true;
}

void
CreateSomeData()
{
  command = CREATE_SOME_DATA;

  for (auto s : senders)
    s->Run();

  for (auto s : senders)
    s->Wait();

  float xmin, xmax, ymin, ymax, zmin, zmax, dmin, dmax;

  if (testdata)
  {
    xmin = ymin = zmin = -1;
    xmax = ymax = zmax =  1;
  }
  else
  {
    senders[0]->GetBox(xmin, ymin, zmin, xmax, ymax, zmax);
    senders[0]->GetDataRange(dmin, dmax);

    for (int i = 1; i < senders.size(); i++)
    {
      float pxmin, pxmax, pymin, pymax, pzmin, pzmax, pdmin, pdmax;
      senders[i]->GetBox(pxmin, pymin, pzmin, pxmax, pymax, pzmax);
      senders[i]->GetDataRange(pdmin, pdmax);

      if (xmin > pxmin) xmin = pxmin;
      if (ymin > pymin) ymin = pymin;
      if (zmin > pymin) zmin = pymin;
      if (xmax < pxmin) xmax = pxmin;
      if (ymax < pymin) ymax = pymin;
      if (zmax < pymin) zmax = pymin;
      if (pdmin < dmin) dmin = pdmin;
      if (pdmax < dmin) dmax = pdmax;
    }
  }

  MPI_Reduce(&xmin, &gxmin, 1, MPI_FLOAT, MPI_MIN, 0, MPI_COMM_WORLD);
  MPI_Reduce(&ymin, &gymin, 1, MPI_FLOAT, MPI_MIN, 0, MPI_COMM_WORLD);
  MPI_Reduce(&zmin, &gzmin, 1, MPI_FLOAT, MPI_MIN, 0, MPI_COMM_WORLD);
  MPI_Reduce(&xmax, &gxmax, 1, MPI_FLOAT, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(&zmax, &gymax, 1, MPI_FLOAT, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(&zmax, &gzmax, 1, MPI_FLOAT, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(&dmin, &gdmax, 1, MPI_FLOAT, MPI_MIN, 0, MPI_COMM_WORLD);
  MPI_Reduce(&dmax, &gdmax, 1, MPI_FLOAT, MPI_MAX, 0, MPI_COMM_WORLD);
}

int
main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  vector<string> pfiles;

  vector<string> hosts;
  hosts.push_back("localhost");

  string master_host("localhost");

  int base_port = 1900;

  for (int i = 1; i < argc; i++)
    if (! strcmp(argv[i], "-b"))
    {
      ifstream ifs(argv[++i]);
      ifs >> gxmin >> gymin >> gzmin >> gxmax >> gymax >> gzmax;
      ifs.close();
    }
    else if (! strcmp(argv[i], "-B"))
    {
      gxmin = atof(argv[++i]);
      gymin = atof(argv[++i]);
      gzmin = atof(argv[++i]);
      gxmax = atof(argv[++i]);
      gymax = atof(argv[++i]);
      gzmax = atof(argv[++i]);
    }
    else if (! strcmp(argv[i], "-M")) 
    {
      master_host = argv[++i];
    }
    else if (! strcmp(argv[i], "-P")) 
    {
      base_port = atoi(argv[++i]);
    }
    else if (! strcmp(argv[i], "-h"))
    {
      hosts.clear();
      ifstream ifs(argv[++i]);
      string s;
      while (ifs >> s)
        hosts.push_back(s);
      ifs.close();
    }
    else if (! strcmp(argv[i], "-p"))
    {
      generated_nPts = atoi(argv[++i]);
    }
    else if (! strcmp(argv[i], "-n"))
    {
      n_senders = atoi(argv[++i]);
    }
    else if (! strcmp(argv[i], "-f"))
    {
      fuzz = atof(argv[++i]);
    }
    else if (! strcmp(argv[i], "-F"))
    {
      ifstream ifs(argv[++i]);
      while (true)
      {
        string part;
        ifs >> part;
        if (ifs.eof())
          break;
        if (part.c_str()[0] != '#')
          pfiles.push_back(part);
      };
      n_senders = pfiles.size();
    }
    else if (! strcmp(argv[i], "-T"))
      testdata = true;
    else if (! strcmp(argv[i], "-V"))
      varname = argv[++i];
    else
      syntax(argv[0]);

  if (n_senders < mpi_size) n_senders = mpi_size;

  int senders_per_rank = n_senders / mpi_size;
  int sender_start = mpi_rank * senders_per_rank;
  int sender_end   = (mpi_rank == mpi_size-1) ? n_senders : (mpi_rank+1) * senders_per_rank;

  ClientSkt *master_socket = (mpi_rank == 0) ? new ClientSkt(master_host, base_port) : NULL;

  for (int i = sender_start;  i < sender_end; i++)
  {
    if (pfiles.size())
      senders.push_back(shared_ptr<SenderThread>(new SenderThread(i, pfiles[i])));
    else
      senders.push_back(shared_ptr<SenderThread>(new SenderThread(i)));
  }

  for (auto s : senders)
  {
    s->SetFuzz(fuzz);
    s->SetBox(gxmin, gymin, gzmin, gxmax, gymax, gzmax);
  }

  CreateSomeData();

  bool done = false;
  char cmd = 's';

  while (! done)
  {
    MPI_Bcast(&cmd, 1, MPI_CHAR, 0, MPI_COMM_WORLD);
    if (cmd == 'q')
      break;

    if (cmd == 's')
    {
      SendSomeData(master_socket);
    }
    else if (cmd == 'c')
    {
      CreateSomeData();
    }
    else if (cmd == 'b')
    {
      float box[6];
      if (mpi_rank == 0)
        cin >> box[0] >> box[1] >> box[2] >> box[3] >> box[4] >> box[5];
      MPI_Bcast(box, 6, MPI_FLOAT, 0, MPI_COMM_WORLD);
      for (auto s : senders)
        s->SetBox(box);
      CreateSomeData();
    }
    else if (cmd == 'p')
    {
      if (mpi_rank == 0)
        cin >> generated_nPts;
      MPI_Bcast(&generated_nPts, 1, MPI_INT, 0, MPI_COMM_WORLD);
      CreateSomeData();
    }
    else if (cmd == 'S')
    {
      CreateSomeData();
      SendSomeData(master_socket);
    }
    else if (cmd == 'k')
    {
      if (mpi_rank == 0)
        if (master_socket->Connect())
        {
          master_socket->Send("quit");
          master_socket->Disconnect();
        }
    }

    if (mpi_rank == 0)
    {
      cout << "? ";
      cin >> cmd;
      if (cin.eof())
          cmd = 'q';
    }
  }

  if (master_socket) delete master_socket;

  MPI_Finalize();
  exit(0);
}
