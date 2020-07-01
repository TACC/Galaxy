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

#pragma once

/*! \file ImageWriter.h 
 * \brief writes PNG format images 
 * \ingroup data
 */

#include <iostream>
#include <string>
#include <png.h>

namespace gxy
{

//! abstract class that writes images
/*! \ingroup data 
 */

class ImageWriter
{
public:
	//! constructor which uses the given string for the image file base name
	/*! \param b the filename base to use for image files, 
	 *          e.g. `foo` produces image filenames `foo_0000.png`, `foo_0001.png`, etc.
	 */
	ImageWriter(std::string b) : basename(b), frame(0) {}
	//! default constructor which uses `frame` for the image file base name
	/*! produces image filenames `frame_0000.png`, `frame_0001.png`, etc.
	 */
 	ImageWriter() : basename("frame"), frame(0) {}

 	//! write a given RGBA array to an image file
 	/*! \param w the image width
 	 * \param h the image height
 	 * \param rgba a buffer of float RGBA values with size `4 * w * h`
	 * \param name the filename to use for this image (if NULL, uses basename and frame count)
	 * \warning assumes existence of alpha channel but ignores its value, using full opacity instead
	 */
	virtual void Write(int w, int h, float *rgba, const char *name=NULL) {}

 	//! write a given RGBA array to an image file
 	/*! \param w the image width
 	 * \param h the image height
 	 * \param rgba a buffer of RGBA values with size `4 * w * h`
	 * \param name the filename to use for this image (if NULL, uses basename and frame count)
	 */
	virtual void Write(int w, int h, unsigned int *rgba, const char *name=NULL) {}

 	//! write a given RGBA array to an image file
 	/*! \param w the image width
 	 * \param h the image height
 	 * \param rgba a buffer of RGBA values with size `4 * w * h`
	 * \param name the filename to use for this image (if NULL, uses basename and frame count)
	 */
	void Write(int w, int h, unsigned char *rgba, const char *name=NULL)
	{
		Write(w, h, (unsigned int *)rgba, name);
	}

protected:

  int frame;
	std::string basename;
};

class ColorImageWriter : public ImageWriter
{
public:
  void Write(int w, int h, float *rgba, const char *name=NULL);
  void Write(int w, int h, unsigned int *rgba, const char *name=NULL);

private:
	static void png_error(png_structp png_ptr, png_const_charp error_msg)
	{
		std::cerr << "PNG error: " << error_msg << std::endl;
	}

	static void png_warning(png_structp png_ptr, png_const_charp warning_msg)
	{
		std::cerr << "PNG error: " << warning_msg << std::endl;
	}

	int write_png(const char *filename, int w, int h, unsigned int *rgba)
	{
		png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL, png_warning, png_error);
		if (!png_ptr)
		{
			std::cerr << "ERROR: Unable to create PNG write structure" << std::endl;
			return 0;
		}

		png_infop info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
		{
			png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
			std::cerr << "ERROR: Unable to create PNG info structure" << std::endl;
			return 0;
		}

		FILE *fp = fopen(filename, "wb");
		if (! fp)
		{
			png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
			std::cerr << "ERROR: Unable to open PNG file: " << filename << std::endl;
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
};

class FloatImageWriter : public ImageWriter
{
public:

	void Write(int w, int h, float *rgba, const char *name=NULL);
  void Write(int w, int h, unsigned int *rgba, const char *name=NULL);
};

}
