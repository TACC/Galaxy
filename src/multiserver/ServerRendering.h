// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
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

#include "Application.h"
#include "Rendering.h"
#include "MultiServerHandler.h"
#include "pthread.h"

/*! \file ServerRendering.h
 *  \brief  ServerRendering is a subclass of the Galaxy renderer's Rendering 
 *          class that knows to ship pixels that arrive from ray processing to
 *          a remote client - eg. the ClientWindow object in a connected client
 *
 * In Galaxy, a Rendering is a class that represents the destination for pixels 
 * from a particular visualization from a particular camera point.  When used in
 * a simple viewer that is NOT multiserver client/server, its role would be to
 * post pixels to a window.   For static rendering to a file, its role would be 
 * to aggregate pixels until the rendering completes, then provide the pixels to
 * be exported to a file.   
 *
 * The ServerRendering is a simple specialization of Rendering that knows 
 * about a socket connetion (the MultiServerHandler) and to ship received 
 * pixel packets across the socket connection to a remote client.
 */

namespace gxy
{

OBJECT_POINTER_TYPES(ServerRendering)

class ServerRendering : public Rendering
{ 
  //! A ServerRendering is a specialization of Rendering that knows how to communicate with a remote ClientWindow
  KEYED_OBJECT_SUBCLASS(ServerRendering, Rendering);
  
public:
  ~ServerRendering(); 

	virtual void initialize();

  //! Overload of AddLocalPixels to ship the pixels across the socket connection
  virtual void AddLocalPixels(Pixel *p, int n, int f, int s);

  //! Set the socket connection (MultiServerHandler)
	void SetHandler(MultiServerHandler *h) { handler = h; }

private:
  pthread_mutex_t lock;
	MultiServerHandler *handler;
	int max_frame;
};
 
} // namesapace gxy
