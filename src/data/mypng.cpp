#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <png.h>

using namespace std;

#if 0
int write_png(const char *filename, int w, int h, unsigned int *rgba)
{return 1;}
#else

void png_error(png_structp png_ptr, png_const_charp error_msg)
{
  cerr << "PNG error: " << error_msg << "\n";
  // longjmp(png_ptr->jmpbuf, 1);
}

void png_warning(png_structp png_ptr, png_const_charp warning_msg)
{
  cerr << "PNG error: " << warning_msg << "\n";
}

int write_png(const char *filename, int w, int h, unsigned int *rgba)
{
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL, png_warning, png_error);
  if (!png_ptr)
  {
    cerr << "Unable to create PNG write structure\n";
    return 0;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    cerr << "Unable to create PNG info structure\n";
    return 0;
  }

  FILE *fp = fopen(filename, "wb");
  if (! fp)
  {
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    cerr << "Unable to open PNG file: " << filename << "\n";
    return 0;
  }

  // if (setjmp(png_ptr->jmpbuf))
  // {
    // fclose(fp);
    // png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    // return 0;
  // }

  png_init_io(png_ptr, fp);

  png_set_IHDR(png_ptr, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGB_ALPHA,
  	PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  png_byte **rows = new png_byte *[h];

  for (int i = 0; i < h; i++)
    rows[i] = (png_bytep)(rgba + i*w);

  png_set_rows(png_ptr, info_ptr, rows);
  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  png_destroy_write_struct(&png_ptr, &info_ptr);
  free(rows);

  fclose(fp);

  return 1;
}
#endif
