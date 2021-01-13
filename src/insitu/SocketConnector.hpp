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

/*! \file SocketConnector.hpp
 *  \brief manage a socket connection between each process and processes of an 
 *  external process - eg. a parallel simulation.   The SocketConnection expects
 *  to receive a set of volumes, each defined on a similar regular grid.
 */

#pragma once

#include <iostream>
#include <sstream>

#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <vtkNew.h>
#include <vtkServerSocket.h>

#include "Application.h"
#include "Datasets.h"
#include "Volume.h"

namespace gxy
{

KEYED_OBJECT_POINTER_TYPES(SocketConnector)

class SocketConnector : public KeyedObject
{
  KEYED_OBJECT_SUBCLASS(SocketConnector, KeyedObject)

  struct variable
  {
    std::string name;
    VolumeDPtr volume;
  };

  struct Waiter
  {
    void Wait()
    {
      while(busy)
        pthread_cond_wait(&cond, &lock);
      pthread_mutex_unlock(&lock);
    }
    
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    bool busy;
  };

public:
  virtual ~SocketConnector();
  virtual void initialize();

  Waiter waiter;

  bool isOpen() { return sskt != NULL; }

  void Wait() { waiter.Wait(); }
  void Open();
  void Accept(VolumeDPtr);
  void Close();

  bool has(std::string);
  Volume* findVariable(std::string name);

  void set_port(int p) { port = p; }
  int get_port() { return port; }

  void set_wait(int w) { wait_time = w; }
  int get_wait() { return wait_time; }

  virtual int serialSize(); 
  virtual unsigned char *serialize(unsigned char *); 
  virtual unsigned char *deserialize(unsigned char *); 

  int getLocalPort() { return local_port; }

  void commit_datasets();

protected:

  bool local_commit(MPI_Comm c);

  bool local_open(MPI_Comm c);
  bool local_close(MPI_Comm c);
  bool local_accept(MPI_Comm c, VolumeDPtr v);

  int wait_time = 100000;  // msec

  std::map<std::string, Volume*> variables;   // local partition

  vtkServerSocket *sskt = NULL;

  int port = 1900;
  int local_port;

  pthread_t socket_t;

  class ConnectionMsg : public Work
  {
  public:
    enum todo {Open, Close, Accept};

    ConnectionMsg(SocketConnector* s, todo t);
    ConnectionMsg(SocketConnector* s, VolumeDPtr v, todo t);

    WORK_CLASS(ConnectionMsg, true);

    bool CollectiveAction(MPI_Comm coll_comm, bool isRoot);
  };
};

}
