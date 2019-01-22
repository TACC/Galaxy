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

/*! \file MultiServerHandler.h
 *  \brief Multiserver is the class that manages the interface between a client and the server
 *
 * A MultiServerHandler is an object within a MultiServer application that manages 
 * a connection to an individual client.  It is created by the MultiServer instance
 * when a client initially connects.   MultiServer then starts a new thread using the
 * its server static method, passing in a pointer to the MSH.  The MS server method
 * stores a pointer to itself in a thread-specific location.  It then recieves
 * the name of a client-specific dynamic library to load, loads it, and
 * launches the DL's server method.   When the DL's server method exits, the MSH
 * RunServer method returns to the MS's server method, which cleans up the DL and
 * pthread_exits.
 */

#pragma once

#include <pthread.h>
#include <string>

#include "GalaxyObject.h"
#include "KeyedObject.h"
#include "SocketHandler.h"
#include "MultiServer.h"
#include "DynamicLibraryManager.h"

namespace gxy
{

class MultiServerHandler;

OBJECT_POINTER_TYPES(MultiServerHandler);

class MultiServerHandler : public SocketHandler
{
  //! MultiServerHandler is a specialization of SocketHandler (which provides the control/data communications across a socket) that nowd about the client's server method and the associated DL.   The DL is owned by the MSH (as well as any objects created by the client code in the server).
  
  GALAXY_OBJECT(MultiServerHandler)

  typedef bool (*server_func)(MultiServerHandler*); //! typedef for server function from the DL

public:
  static MultiServerHandler *GetTheThreadMultiServerHandler(); //! return the thread-specific handler

  MultiServerHandler() : SocketHandler() {}
  MultiServerHandler(int cfd, int dfd) : SocketHandler(cfd, dfd) {}
  MultiServerHandler(std::string host, int port) : SocketHandler(host, port) {}

  bool RunServer(); //! Load the DL, run its server method.

  DynamicLibraryP GetTheDynamicLibrary() { return dlp; }  //! Get the MSH's DynamicLibraryP

  bool handle(char *); // Handle commands not recognized by application's server
  
private:

  DynamicLibraryP dlp;
};

}
