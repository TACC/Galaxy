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

#pragma once

#include "Pixel.h"
#include "MultiServerSocket.h"

#include <pthread.h>

namespace gxy
{

class ClientWindow 
{ 
private:
  static void *rcvr_thread(void *);
	static void *ager_thread(void *p);
  
public:
  ClientWindow(int, int);
	~ClientWindow();

  bool Connect(const char* host, int port);

  void Resize(int w, int h);
	virtual void FrameBufferAger();

  void partial_frame_test()
  {
    if (number_of_partial_frames == -1)
    {
      if (getenv("GXY_N_PARTIAL_FRAMES"))
        number_of_partial_frames = atoi(getenv("GXY_N_PARTIAL_FRAMES"));
      else
        number_of_partial_frames = 10;
    }

    save_partial_updates = true;

    partial_frame_pixel_count = this_frame_pixel_count;
    partial_frame_pixel_delta = (partial_frame_pixel_count / number_of_partial_frames) + 1;
    next_partial_frame_pixel_count = partial_frame_pixel_delta;
    current_partial_frame_count = 0;

    Clear();
  }

  void Clear();
              
	void SetMaxAge(float m, float f = 1) { max_age = m; fadeout = f; }
  float *GetPixels() { return pixels; }
  int GetWidth() { return width; }
  int GetHeight() { return height; }

  MultiServerSocket *GetSocket() { return socket; }

private:
  MultiServerSocket *socket;
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
