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

/*! \file MultiServer.h 
 *  \brief Multiserver is the class that manages a set of connections between
 *  a server and any number of remote clients
 *
 * MultiServer is the main manager for a multiserver application.   It opens
 * a port and watches it for incoming connections from attaching clients.
 * When a connection is made, it establishes two sockets to communicate with
 * with the client - one for control and one for data. It then creates a 
 * MultiServerHandler for the new client, and starts a thread using its own
 * server method for it.
 */
#pragma once

#include <pthread.h>
#include <string>
#include <map>

#include "DynamicLibraryManager.h"

namespace gxy
{

OBJECT_POINTER_TYPES(MultiServer)

extern MultiServerP theMultiServer;

class MultiServer : public GalaxyObject
{
  //! MultiServer is a subclass of GalaxyObject so that it is ref counted.
  GALAXY_OBJECT(MultiServer)

public:
  static void Register();

  //! Method to create a MultiServer object and return a shared pointer to it.
  static MultiServerP NewP()
  {
    theMultiServer = MultiServerP(new MultiServer);
    return theMultiServer;
  }

  //! Get the solitary global MultiServer object
  static MultiServerP Get() { return theMultiServer; }

  //! Start the MultiServer waiting on a port
  //! \param port the port to use
  void Start(int p);

  //! save a KeyedObject as a global
  /*! \param name the name under which the object is saved
   *  \param obj the object to save
   */
  void SetGlobal(std::string name, KeyedObjectP obj) { globals[name] = obj; }

  //! retrieve a global KeyedObject.  NULL if it isn't there
  /*! \param name the name under which the object is saved
   *  \param obj the object to save
   */
  KeyedObjectP GetGlobal(std::string name) { return globals[name]; }

  //! delete global reference to a global KeyedObject.
  /*! \param name the name under which the object is saved
   */
  void DropGlobal(std::string name);

  //! clear list of globally accessable objects
  void ClearGlobals() { globals.clear(); }

  //! return string of semicolon separated global object names
  std::string GetGlobalNames();

  DynamicLibraryManager *getTheDynamicLibraryManager() { return dynamicLibraryManager; }


private:
  MultiServer();
  static void* watch(void *);
  static void* start(void *);

public:
  ~MultiServer();

  //! Called from anywhere to get the MultiServer to clean up and quit
  void Quit() { done = true; }

private:
  int port;
  int fd;
  bool done;
  pthread_t watch_tid;

  DynamicLibraryManager *dynamicLibraryManager;

  std::map<std::string, KeyedObjectP> globals;
};

}
