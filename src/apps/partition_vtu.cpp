#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

using namespace rapidjson;

#include "Box.h"

#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkUnstructuredGrid.h>
#include <vtkLine.h>
#include <vtkTriangle.h>
#include <vtkQuad.h>
#include <vtkPolyLine.h>
#include <vtkTriangleStrip.h>
#include <vtkCellArray.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkUnsignedCharArray.h>

using namespace gxy;

int
main(int argc, char **argv)
{
  float gzone = 0.0;
  string pdoc = "";
  vector<string> vtus;

  for (int i = 1; i < argc; i++)
    if (! strcmp(argv[i], "-g")) gzone = atof(argv[++i]);
    else if (pdoc == "") pdoc = argv[i];
    else vtus.push_back(argv[i]);

  if (pdoc == "" || vtus.size() == 0)
  {
    cerr << "syntax: " << argv[0] << " [-g] pdoc vtu ...\n";
    exit(1);
  }

  ifstream ifs(pdoc);
  if (! ifs)
  {
    std::cerr << "could not open pdoc\n";
    exit(1);
  }

  IStreamWrapper isw(ifs);

  Document doc;
  doc.ParseStream(isw);

  if (! doc.HasMember("parts"))
  {
    std::cerr << "invalid partition document\n";
    exit(1);
  }

  Value& parts = doc["parts"];

  vector<Box> boxes;
  for (int i = 0; i < parts.Size(); i++)
  {
    Value& e = parts[i]["extent"];
    Box b;
    b.LoadFromJSON(e);
    b.grow(gzone);
    boxes.push_back(b);
  }

  for (auto s : vtus)
  {
    vtkXMLUnstructuredGridReader *rdr = vtkXMLUnstructuredGridReader::New();
    rdr->SetFileName(s.c_str());
    rdr->Update();
    
    vtkUnstructuredGrid *vtu = rdr->GetOutput();
    int ncells = vtu->GetNumberOfCells();

    float *points; int npoints;
    vtkFloatArray *fa = vtkFloatArray::SafeDownCast(vtu->GetPoints()->GetData());
    if (fa)
    {
      points = (float *)fa->GetVoidPointer(0);
      npoints = fa->GetNumberOfTuples();
    }
    else
    {
      vtkDoubleArray *da = vtkDoubleArray::SafeDownCast(vtu->GetPoints()->GetData());
      if (da)
      {
        npoints = da->GetNumberOfTuples();
        points = new float[da->GetNumberOfTuples() * 3];
        for (int i = 0; i < da->GetNumberOfTuples() * 3; i++)
          points[i] = ((double *)da->GetVoidPointer(0))[i];
      }
      else
      {
        cerr << "points have to be float or double\n";
        exit(1);
      }
    }

    for (int p = 0; p < boxes.size(); p++)
    {
      Box b = boxes[p];

      vtkCellArray *ocells = vtkCellArray::New();
      vector<int> ctypes;

      vector<vtkIdType> cmap;
      vtkIdType pmap[npoints];
      for (int i = 0; i < npoints; i++)
        pmap[i] = 99999999;

      int nxtp = 0;

      for (vtkIdType c = 0; c < ncells; c++)
      {
        const vtkIdType *cell_points;
        vtkIdType n_cell_points;

        vtu->GetCellPoints(c, n_cell_points, cell_points);

        int offset = 0;
        if (vtu->GetCellType(c) == VTK_POLY_LINE || vtu->GetCellType(c) == VTK_TRIANGLE_STRIP)
        {
          bool insiders[n_cell_points];
          for (int p = 0; p < n_cell_points; p++)
          {
            vec3f vertex = vec3f(points+3*cell_points[p]);
            insiders[p] = b.isIn(vertex);
          }

          for (int v = 0; v < n_cell_points; v++)
          {
            int s = v;
            while (s < n_cell_points && !insiders[s])
              s++;

            if (s == n_cell_points)
              break;

            if (s > 0) s --;      // include initial outside point

            int e = s + 1;
            while (e < n_cell_points && insiders[e])  // breaks at first outsider
              e++;

            if (e >= n_cell_points) e = n_cell_points - 1;

            int n = (e - s) + 1;
            
            vtkCell *cell = (vtu->GetCellType(c) == VTK_POLY_LINE) ? (vtkCell*)vtkPolyLine::New() : (vtkCell*)vtkTriangleStrip::New();
            cell->GetPointIds()->SetNumberOfIds(n);
            for (int i = 0; i < n; i++)
            {
              vtkIdType id = cell_points[i+s];

              if (pmap[id] == 99999999)
                pmap[id] = nxtp++;

              cell->GetPointIds()->SetId(i, pmap[id]);
            }

            ocells->InsertNextCell(cell);
            ctypes.push_back(vtu->GetCellType(c));
            cmap.push_back(c);

            v = e;
          }
        } 
        else if (vtu->GetCellType(c) == VTK_LINE || vtu->GetCellType(c) == VTK_TRIANGLE || vtu->GetCellType(c) == VTK_QUAD)
        {
          const vtkIdType *cell_points;
          vtkIdType n_cell_points;

          vtu->GetCellPoints(c, n_cell_points, cell_points);

          bool hit = false;
          for (int i = 0; i < n_cell_points && !hit; i++)
          {
            vtkIdType id = cell_points[i];
            vec3f vertex = vec3f(points+3*id);
            hit = b.isIn(vertex);
          }

          if (hit)
          {
            for (int i = 0; i < n_cell_points; i++)
            {
              int id = cell_points[i];
              if (pmap[id] == 99999999)
                pmap[id] = nxtp++;
            }
  
            vtkCell *icell;
            vtkCell *ocell;

            icell = vtu->GetCell(c);

            if (vtu->GetCellType(c) == VTK_LINE)
              ocell = vtkLine::New();
            else if (vtu->GetCellType(c) == VTK_TRIANGLE)
              ocell = vtkTriangle::New();
            else
              ocell = vtkQuad::New();

            ocell->GetPointIds()->SetNumberOfIds(icell->GetPointIds()->GetNumberOfIds());
            for (int i = 0; i < icell->GetPointIds()->GetNumberOfIds(); i++)
              ocell->GetPointIds()->SetId(i, pmap[icell->GetPointIds()->GetId(i)]);

            ocells->InsertNextCell(ocell);
            ctypes.push_back(vtu->GetCellType(c));
            cmap.push_back(c);
          }
        }
      }

      vtkUnstructuredGrid *ovtu = vtkUnstructuredGrid::New();

      ovtu->SetCells(ctypes.data(), ocells);

      vtkFloatArray *pa = vtkFloatArray::New();
      pa->SetNumberOfComponents(3);
      pa->SetNumberOfTuples(nxtp);

      for (int i = 0; i < npoints; i++)
        if (pmap[i] != 99999999)
        {
          float *src = points + i*3;
          float *dst = ((float *)pa->GetVoidPointer(0)) + pmap[i]*3;
          for (int j = 0; j < 3; j++)
            *dst++ = *src++;
        }

      vtkPoints *opts = vtkPoints::New();
      opts->SetData(pa);
      ovtu->SetPoints(opts);

      vtkPointData *pd = vtu->GetPointData();
      for (int pa = 0; pa < pd->GetNumberOfArrays(); pa++)
      {
        int nc = pd->GetArray(pa)->GetNumberOfComponents();

        vtkFloatArray *sa = vtkFloatArray::SafeDownCast(pd->GetArray(pa));
        if (sa)
        {
          vtkFloatArray *da = vtkFloatArray::New();
          da->SetName(pd->GetArray(pa)->GetName());
          da->SetNumberOfComponents(nc);
          da->SetNumberOfTuples(nxtp);
          float *dd = (float *)da->GetVoidPointer(0);

          float *sd = (float *)sa->GetVoidPointer(0);
          for (int i = 0; i < npoints; i++)
            if (pmap[i] != 99999999)
            {
              float *s = sd + i*nc;
              float *d = dd + pmap[i]*nc;
              for (int j = 0; j < nc; j++)
                *d++ = *s++;
            }

          ovtu->GetPointData()->AddArray(da);
        }
        else
        {
          vtkDoubleArray *sa = vtkDoubleArray::SafeDownCast(pd->GetArray(pa));
          if (sa)
          {
            vtkFloatArray *da = vtkFloatArray::New();
            da->SetName(pd->GetArray(pa)->GetName());
            da->SetNumberOfComponents(nc);
            da->SetNumberOfTuples(nxtp);
            float *dd = (float *)da->GetVoidPointer(0);

            double *sd = (double *)sa->GetVoidPointer(0);
            for (int i = 0; i < npoints; i++)
              if (pmap[i] != 99999999)
              {
                double *s = sd + i*nc;
                float *d = dd + pmap[i]*nc;
                for (int j = 0; j < nc; j++)
                  *d++ = *s++;
              }

            ovtu->GetPointData()->AddArray(da);
          }
        }
      }

      vtkCellData *cd = vtu->GetCellData();
      for (int pa = 0; pa < cd->GetNumberOfArrays(); pa++)
      {
        int nc = cd->GetArray(pa)->GetNumberOfComponents();

        vtkFloatArray *sa = vtkFloatArray::SafeDownCast(cd->GetArray(pa));
        if (sa)
        {
          vtkFloatArray *da = vtkFloatArray::New();
          da->SetName(cd->GetArray(pa)->GetName());
          da->SetNumberOfComponents(nc);
          da->SetNumberOfTuples(nxtp);
          float *dd = (float *)da->GetVoidPointer(0);

          float *sd = (float *)sa->GetVoidPointer(0);
          for (int i = 0; i < npoints; i++)
            if (pmap[i] != 99999999)
            {
              float *s = sd + i*nc;
              float *d = dd + pmap[i]*nc;
              for (int j = 0; j < nc; j++)
                *d++ = *s++;
            }

          ovtu->GetCellData()->AddArray(da);
        }
        else
        {
          vtkDoubleArray *sa = vtkDoubleArray::SafeDownCast(cd->GetArray(pa));
          if (sa)
          {
            vtkFloatArray *da = vtkFloatArray::New();
            da->SetName(cd->GetArray(pa)->GetName());
            da->SetNumberOfComponents(nc);
            da->SetNumberOfTuples(nxtp);
            float *dd = (float *)da->GetVoidPointer(0);

            double *sd = (double *)sa->GetVoidPointer(0);
            for (int i = 0; i < npoints; i++)
              if (pmap[i] != 99999999)
              {
                double *s = sd + i*nc;
                float *d = dd + pmap[i]*nc;
                for (int j = 0; j < nc; j++)
                  *d++ = *s++;
              }
          ovtu->GetCellData()->AddArray(da);
          }
        }
      }

      vtkXMLUnstructuredGridWriter *wrtr = vtkXMLUnstructuredGridWriter::New();
      
      char oname[256];
      sprintf(oname, "%s-%d.vtu", s.substr(0, s.find('.')).c_str(), p);
      wrtr->SetDataModeToAscii();
      wrtr->SetInputData(ovtu);
      wrtr->SetFileName(oname);
      wrtr->Write();
      wrtr->Delete();
    }

    if (! fa)
      delete[] points;
  }
}

