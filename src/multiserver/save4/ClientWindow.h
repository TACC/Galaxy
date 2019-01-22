// ========================================================================== //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 //
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


//! ClientWindow is a socket-connected endpoint for receiving pixels
/*! 
 * ClientWindow is a class that provides a socket-connected
 * endpoint for receiving pixels from a remote renderer, maintaining
 * a asynchronously updated image buffer.
 * 
 * A viewer using this class will typically create an actual window
 * that will present the image, then a) watch the window for user
 * interactions that cause re-rendering, and b) update the actual
 * window pixels from the image buffer maintained by the ClientWindow.
 * 
 * The ClientWindow manages asynchronous update by using several 
 * pixel-resolution buffers:
 * 
 * pixels, which contains the current pixel contents of the window
 * as (r,g,b,a) floats
 * 
 * - frameids, which contains the most advanced frame ID encountered
 *   for each pixel. This is used to recognize new frames and 
 *   therefore the new pixels will replace, rather than accumlate 
 *   with, prior values
 * 
 * - negative_pixels, containing the sum of any negative pixels that
 *   arrive before the first positive values for the corresponding pixel
 * 
 * - negative_frameids, which are to negative_pixels as frameids are to pixels
 * 
 * - frame_times, holding the most recent modification time for each pixel
 * 
 * The ClientWindow implements two threads - the rcvr_thread, which watches the input
 * socket connection and modifies the above buffers when pixel messages arrive, and
 * the ager_thread, which runs every tenth of a second, looking for pixels that have 
 * not been updated recently and can be faded out.
 */

#pragma once

#include "Pixel.h"
#include "SocketHandler.h"

#include <pthread.h>

namespace gxy
{

class ClientWindow 
{ 
private:
  //! Thread that watches data sockets for incoming pixel messages 
  static void *rcvr_thread(void *);

  //! Thread that manages aging not-recently-updated pixels out
	static void *ager_thread(void *p);
  
public:
  //! Creator with width and height
  ClientWindow(int, int);

  //! Destructor
	~ClientWindow();

  //! Called to establish connection with pixel source.  Blocking.  
  bool Connect(const char* host, int port);

  //! Resize frame buffers
  void Resize(int w, int h);

  //! Virtual function called by the ager thread to implement the actual aging algorithm
	virtual void FrameBufferAger();

  //! Toggle the saving of partial frame buffer contents to a file.   If the GXY_N_PARTIAL_FRAMES env var is set, it holds the number of partial frames to save for each full frame, defaulting to 10.
  void partial_frame_test()
  {
    if (number_of_partial_frames == -1)
    {
      if (getenv("GXY_N_PARTIAL_FRAMES"))
        number_of_partial_frames = atoi(getenv("GXY_N_PARTIAL_FRAMES"));
      else
        number_of_partial_frames = 10;

      current_partial_frame_count = 0;
      partial_frame_pixel_count = this_frame_pixel_count;
      partial_frame_pixel_delta = (partial_frame_pixel_count / number_of_partial_frames) + 1;
      next_partial_frame_pixel_count = partial_frame_pixel_delta;
    }

    save_partial_updates != save_partial_updates;
    
    Clear();
  }

  //! Clear the buffers
  void Clear();
              
  //! Set parameters on the ager.  
  //! \param m the number of seconds to retain the pixels before fading begins
  //! \param fadeout the number of seconds that fading out will take
	void SetMaxAge(float m, float f = 1) { max_age = m; fadeout = f; }

  //! Get a pointer to the current pixel buffer
  float *GetPixels() { return pixels; }

  //! Get width of image buffer
  int GetWidth() { return width; }

  //! Get height of image buffer
  int GetHeight() { return height; }

  //! Get socket being used to pass pixel messages
  SocketHandler *GetSocket() { return socket; }

private:
  SocketHandler *socket;
  int width, height;

  void AddPixels(Pixel *, int, int);

	long	      my_time();
	long        t_start;

	float	      max_age, fadeout;

  bool        save_partial_updates;
  int         number_of_partial_frames;
  int         current_partial_frame_count;
  int         partial_frame_pixel_count;
  int         this_frame_pixel_count;
  int         partial_frame_pixel_delta;
  int         next_partial_frame_pixel_count;

	float*      pixels = NULL;
	float*      negative_pixels = NULL;
	int*        frameids = NULL;
	int*        negative_frameids = NULL;
	long*       frame_times = NULL;

	pthread_t	  ager_tid;
	pthread_t	  rcvr_tid;
	bool 			  kill_threads;

	int         current_frame;

	pthread_mutex_t lock;
};
 

} // namespace gxy
