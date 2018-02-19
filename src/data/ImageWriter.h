#pragma once

#include <iostream>
#include <string>

using namespace std;

#include <png.h>

class ImageWriter
{
public:
	ImageWriter(string b) : basename(b), frame(0) {}
	ImageWriter() : basename("frame"), frame(0) {}

	void Write(int w, int h, float *rgba, const char *name=NULL)
	{
		unsigned char *buf = new unsigned char[w*h*4];
		float *p = rgba; unsigned char *b = buf;
		for (int i = 0; i < w*h; i++)
		{
			*b++ = (unsigned char)(255*(*p++));
			*b++ = (unsigned char)(255*(*p++));
			*b++ = (unsigned char)(255*(*p++));
			*b++ = 0xff ; p ++;
		}
		Write(w, h, (unsigned int *)buf);
		delete[] buf;
	}

	void Write(int w, int h, unsigned int *rgba, const char *name=NULL)
	{
		if (name)
			write_png(name, w, h, (unsigned int *)rgba);
		else
		{
			char filename[1024];
			sprintf(filename, "%s_%04d.png", basename.c_str(), frame++);
			write_png(filename, w, h, (unsigned int *)rgba);
		}
	}

	void Write(int w, int h, unsigned char *rgba, const char *name=NULL)
	{
		Write(w, h, (unsigned char *)rgba);
	}

private:

	static void png_error(png_structp png_ptr, png_const_charp error_msg)
	{
		cerr << "PNG error: " << error_msg << "\n";
	}

	static void png_warning(png_structp png_ptr, png_const_charp warning_msg)
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

  int frame;
	string basename;
};
