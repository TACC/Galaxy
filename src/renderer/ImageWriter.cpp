// ========================================================================== //
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

#include <iostream>
#include <fstream>
#include <sstream>

#include "ImageWriter.h"

namespace gxy
{

void 
ColorImageWriter::Write(int w, int h, float *rgba, const char *name)
{ 
  unsigned char *buf = new unsigned char[w*h*4];
  float *p = rgba;  
  for (int y = 0; y < h; y++)
  { 
    unsigned char *b = buf + ((h-1)-y)*w*4;
    for (int x = 0; x < w; x++)
    { 
// GDA
#if 0
      if (p[3] < 1)
      {
        *b++ = (unsigned char)(255*0.75);
        *b++ = (unsigned char)(255*0.75);
        *b++ = (unsigned char)(255*0.75);
        p += 3;
      } else {
        *b++ = (unsigned char)(255*(*p++));
        *b++ = (unsigned char)(255*(*p++));
        *b++ = (unsigned char)(255*(*p++));
      }
#else
      *b++ = (unsigned char)(255*(*p++));
      *b++ = (unsigned char)(255*(*p++));
      *b++ = (unsigned char)(255*(*p++));
#endif
      *b++ = 0xff ; p ++;
    }
  }
  Write(w, h, (unsigned int *)buf, name);
  delete[] buf;
}

void 
ColorImageWriter::Write(int w, int h, unsigned int *rgba, const char *name)
{ 

  if (name)
  {
    char filename[1024];
    sprintf(filename, "%s.png", name);
    write_png(filename, w, h, (unsigned int *)rgba);
  }
  else
  {
    char filename[1024];
    sprintf(filename, "%s.png", basename.c_str());
    write_png(filename, w, h, (unsigned int *)rgba);
  }

}

#define APPEND(a, b)                                    \
{                                                       \
  std::stringstream sstr;                               \
  sstr << a << b;                                       \
  std::string str = sstr.str();                         \
  str.insert(str.size(), 80-str.size(), ' ');           \
  header = header + str;                                \
}

#define SWAP(a, b) a[3] = b[0], a[2] = b[1], a[1] = b[2], a[0] = b[3]

void
FloatImageWriter::Write(int w, int h, float *rgba, const char *name)
{
  std::string fn;
  std::ofstream f;

  if (name)
    fn = name;
  else 
    fn = basename;

  std::string header("");
  APPEND("SIMPLE  = ", "T");
  APPEND("BITPIX  = ", -32);
  APPEND("NAXIS   = ", 2);
  APPEND("NAXIS1  = ", w);
  APPEND("NAXIS2  = ", h);
  APPEND("EXTEND  = ", "T");
  APPEND("END", "")
  header.insert(header.size(), 2880-header.size(), ' ');

  unsigned char *rbuf = new unsigned char[w*h*4];
  unsigned char *gbuf = new unsigned char[w*h*4];
  unsigned char *bbuf = new unsigned char[w*h*4];
  unsigned char *abuf = new unsigned char[w*h*4];

  unsigned char *rptr = rbuf;
  unsigned char *gptr = gbuf;
  unsigned char *bptr = bbuf;
  unsigned char *aptr = abuf;
  unsigned char *rgbaptr = (unsigned char *)rgba;
  for (int i = 0; i < w*h; i++)
  {
    SWAP(rptr, rgbaptr), rptr += 4, rgbaptr += 4;
    SWAP(gptr, rgbaptr), gptr += 4, rgbaptr += 4;
    SWAP(bptr, rgbaptr), bptr += 4, rgbaptr += 4;
    SWAP(aptr, rgbaptr), aptr += 4, rgbaptr += 4;
  }
    
  int padsz = 2880 - (w*h*4 % 2880);
  char *pad = new char[padsz];

  for (auto i = 0; i < padsz; i++)
    pad[i] = ' ';

#define WRITE_CHANNEL(channel, buffer)                                  \
  {                                                                     \
    char fullname[1024];                                                \
    sprintf(fullname, "%s_%c.fits", fn.c_str(), channel);               \
    f.open(fullname, std::ios::out | std::ios::binary);                 \
    if (!f.good())                                                      \
    {                                                                   \
      std::cerr << "error opening file: " << fullname << "\n";          \
      return;                                                           \
    }                                                                   \
    f.write(header.c_str(), 2880);                                      \
    f.write((char *)buffer, w*h*4);                                     \
    f.write(pad, padsz);                                                \
    f.close();                                                          \
  }

  WRITE_CHANNEL('r', rbuf);
  WRITE_CHANNEL('g', gbuf);
  WRITE_CHANNEL('b', bbuf);
  WRITE_CHANNEL('a', abuf);

  frame++;
  delete[] rbuf;
  delete[] gbuf;
  delete[] bbuf;
  delete[] abuf;
}

void
FloatImageWriter::Write(int w, int h, unsigned int *rgba, const char *name)
{ 
  std::cerr << "cannot wtite int image buffer to float image file\n";
}

}
