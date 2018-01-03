#include <mpi.h>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>

#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkFloatArray.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkXMLMultiBlockDataWriter.h"

using namespace std;

void
factor(int n, int *factors)
{
    int k = (int)(pow((double)n, 0.333) + 1);

    int i;
    for (i = k; i > 1; i--)
        if (n%i == 0) break;

    n /= i;
    k = (int)(pow((double)n, 0.5) + 1);

    int j;
    for (j = k; j > 1; j--)
        if (n%j == 0) break;

    factors[0] = i;
    factors[1] = j;
    factors[2] = n/j;
}

void
syntax(char *a)
{
    cerr << "syntax: " << a << " [options] (to stdout)\n";
    cerr << "options:\n";
    cerr << "  -r xres yres zres  overall grid resolution (256x256x256)\n";
    cerr << "  -t dt nt           time series delta, number of timesteps (0, 1)\n";
    cerr << "  -m r s             set rank, size (for testing)\n";
    exit(1);
}

int main(int argc, char *argv[])
{
  int xsz = 256, ysz = 256, zsz = 256;
  float t = 3.1415926;
  float delta_t = 0;
  int   nt = 1;
  int   ascii = 0;
  int		mpir, mpis;
	int 	np = -1;

	MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &mpis);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpir);


  for (int i = 1; i < argc; i++)
    if (argv[i][0] == '-') 
      switch(argv[i][1])
      {
				case 'n': np = atoi(argv[++i]); break;
				case 'a': ascii = 1; break;
				case 'm': mpir = atoi(argv[++i]);
									mpis = atoi(argv[++i]); break;
				case 'r': xsz = atoi(argv[++i]);
									ysz = atoi(argv[++i]);
									zsz = atoi(argv[++i]); break;
				case 't': delta_t = atof(argv[++i]); nt = atoi(argv[++i]); break;
				default: 
					syntax(argv[0]);
      }
    else
      syntax(argv[0]);

  if (np == -1)
		np = mpis;

  int sz = (xsz > ysz) ? xsz : ysz;
  sz = (zsz > sz) ? zsz : sz;

  int factors[3], nparts;
  factor(np, factors);
  nparts = factors[0]*factors[1]*factors[2];

  float d = 2.0 / (sz-1);

  int dx = ((xsz + factors[0])-1) / factors[0];
  int dy = ((ysz + factors[1])-1) / factors[1];
  int dz = ((zsz + factors[2])-1) / factors[2];

  // Just used on rank 0
  ofstream pvti;

  for (int t = 0; t < nt; t++)
  {
    if (mpis > 1 && mpir == 0)
		{
		  char pvti_filename[256];
			sprintf(pvti_filename, "radial-%d.pvti", t);
			pvti.open(pvti_filename);

			pvti << "<?xml version=\"1.0\"?>\n";
			pvti << "<VTKFile type=\"PImageData\" version=\"0.1\" "
														<< "byte_order=\"LittleEndian\" compressor=\"vtkZLibDataCompressor\">\n";
			pvti << "  <PImageData WholeExtent=\" 0 " 
														<< (zsz-1) << " 0 " << (ysz-1) << " 0 " << (xsz-1) 
														<< "\" GhostLevel=\"1\" Origin=\"-1 -1 -1\" "
														<< " Spacing=\"" << d << " " << d << " " << d << "\">\n";
			pvti << "    <PPointData Scalars=\"oneBall\" Vectors=\"vector\">\n";
			pvti << "      <PDataArray type=\"Float32\" Name=\"xramp\"/>\n";
			pvti << "      <PDataArray type=\"Float32\" Name=\"yramp\"/>\n";
			pvti << "      <PDataArray type=\"Float32\" Name=\"zramp\"/>\n";
			pvti << "      <PDataArray type=\"Float32\" Name=\"oneBall\"/>\n";
			pvti << "      <PDataArray type=\"Float32\" Name=\"eightBalls\"/>\n";
			pvti << "      <PDataArray type=\"Float32\" Name=\"vector\" NumberOfComponents=\"3\"/>\n";
		  pvti << "    </PPointData>\n";
    }

		float T = 0.1 + 0.9 * cos(t*delta_t);

		int ix = 0, iy = 0, iz = 0;
		for (int part = 0; part < nparts; part++)
		{
			
      int sx = ix * dx,
          sy = iy * dy,
          sz = iz * dz;

      int ex = (ix == (factors[0]-1)) ? xsz-1 : sx + dx,
          ey = (iy == (factors[1]-1)) ? ysz-1 : sy + dy,
          ez = (iz == (factors[2]-1)) ? zsz-1 : sz + dz;

			sx = (ix == 0) ? sx : sx-1;
			sy = (iy == 0) ? sy : sy-1;
			sz = (iz == 0) ? sz : sz-1;

			ex = (ix == (factors[0]-1)) ? ex : ex+1;
			ey = (iy == (factors[1]-1)) ? ey : ey+1;
			ez = (iz == (factors[2]-1)) ? ez : ez+1;

			char vti_partname[256];
			if (mpis > 1)
				sprintf(vti_partname, "part-%d-%d.vti", t, part);
			else
				sprintf(vti_partname, "radial-%d.vti", t);

			if (mpis > 1 && mpir == 0)
				pvti << "<Piece Extent=\"" << sz << " " << ez << " " << sy << " " << ey << " " << sx << " " << ex << "\" Source=\"" << vti_partname << "\"/>\n";

			if ((part % mpis) == mpir)
			{
				int lxsz = (ex - sx) + 1,
						lysz = (ey - sy) + 1,
						lzsz = (ez - sz) + 1;

				int np = lxsz*lysz*lzsz;
				float *xramp = new float[np];
				float *yramp = new float[np];
				float *zramp = new float[np];
				float *oneBall = new float[np];
				float *eightBalls = new float[np];
				float *vector = new float[3*np];
				float *x = xramp;
				float *y = yramp;
				float *z = zramp;
				float *g = vector;
				float *o = oneBall;
				float *e = eightBalls;
				for (int i = 0; i < lzsz; i++)
				{
					float Z = -1 + (i+sz)*d;
					for (int j = 0; j < lysz; j++)
					{
						float Y = -1 + (j+sy)*d;
						for (int k = 0; k < lxsz; k++)
						{
							float X = -1 + (k+sx)*d;
							*x++ = X;
							*y++ = Y;
							*z++ = Z;
							float m = sqrt(X*X + Y*Y + Z*Z);
							*o++ = T * m;
							float dx = ((X < 0) ? -X : X) - 0.5;
							float dy = ((Y < 0) ? -Y : Y) - 0.5;
							float dz = ((Z < 0) ? -Z : Z) - 0.5;
							m = sqrt(dx*dx + dy*dy + dz*dz);
							*e++ = T * m;
							*g++ = X / m;
							*g++ = Y / m;
							*g++ = Z / m;
						}
					}
				}

				vtkImageData *id = vtkImageData::New();
				id->Initialize();
				id->SetExtent(sz, ez, sy, ey, sx, ex);
				id->SetSpacing(d, d, d);
				id->SetOrigin(-1.0 + d*sz, -1.0 + d*sy, -1.0 + d*sx);

				vtkFloatArray *xRampArray = vtkFloatArray::New();
				xRampArray->SetNumberOfComponents(1);
				xRampArray->SetArray(xramp, np, 1);
				xRampArray->SetName("xramp");
				id->GetPointData()->AddArray(xRampArray);
				xRampArray->Delete();

				vtkFloatArray *yRampArray = vtkFloatArray::New();
				yRampArray->SetNumberOfComponents(1);
				yRampArray->SetArray(yramp, np, 1);
				yRampArray->SetName("yramp");
				id->GetPointData()->AddArray(yRampArray);
				yRampArray->Delete();

				vtkFloatArray *zRampArray = vtkFloatArray::New();
				zRampArray->SetNumberOfComponents(1);
				zRampArray->SetArray(zramp, np, 1);
				zRampArray->SetName("zramp");
				id->GetPointData()->AddArray(zRampArray);
				zRampArray->Delete();

				vtkFloatArray *oneBallArray = vtkFloatArray::New();
				oneBallArray->SetNumberOfComponents(1);
				oneBallArray->SetArray(oneBall, np, 1);
				oneBallArray->SetName("oneBall");
				id->GetPointData()->SetScalars(oneBallArray);
				oneBallArray->Delete();

				vtkFloatArray *eightBallsArray = vtkFloatArray::New();
				eightBallsArray->SetNumberOfComponents(1);
				eightBallsArray->SetArray(eightBalls, np, 1);
				eightBallsArray->SetName("eightBalls");
				id->GetPointData()->AddArray(eightBallsArray);
				eightBallsArray->Delete();

				vtkFloatArray *vectors = vtkFloatArray::New();
				vectors->SetNumberOfComponents(3);
				vectors->SetArray(vector, 3*np, 1);
				vectors->SetName("vector");
				id->GetPointData()->SetVectors(vectors);
				vectors->Delete();

				vtkXMLImageDataWriter *wr = vtkXMLImageDataWriter::New();
				wr->SetInputData(id);
				wr->SetFileName(vti_partname);

				if (ascii)
					wr->SetDataModeToAscii();
				else
					wr->SetDataModeToBinary();

				wr->Update();
				wr->Write();
				wr->Delete();
				id->Delete();
			}

			if (++ix == factors[0])
			{
				ix = 0;
				if (++iy == factors[1])
				{
					iy = 0;
					iz++;
				}
			}
		}

    if (mpis > 1 && mpir == 0)
		{
			std::cerr << "writing timestep " << t << "\n";
			pvti << "    </PImageData>\n";
			pvti << "</VTKFile>\n";
			pvti.close();
		}
  }

  MPI_Finalize();
}
