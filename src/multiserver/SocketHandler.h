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

/*! \file SocketHandler.h
 *  \brief SocketHandler handles comminications between a client and a server,
 *  using two connections: one for control and the other for data.   A locking
 *  mechanism is used so that multi-part messages and send/receive pairs are
 *  safe.
 */

#pragma once

#include <pthread.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

namespace gxy
{

class SocketHandler
{
public:
  SocketHandler();

  //! Creator when control and data sockets are already available
  //! \param cfd file descriptor for control socket
  //! \param dfd file descriptor for data socket
  SocketHandler(int cfd, int dfd);

  //! Creator to attempt to open a connection to a remote server
  //! \param host name of host to connect to
  //! \param port port to connect to
  SocketHandler(std::string host, int port);

  ~SocketHandler();

  //! Given a set of memory pointers and sizes, send them as a contiguous multi-part message using the control socket
  bool CSendV(char** buf, int* size)
  {
     pthread_mutex_lock(&c_lock);
     bool b = SendV(control_fd, buf, size);
     pthread_mutex_unlock(&c_lock);
     return b;
  }

  //! Given a memory pointer and a size, send a message using the control socket
  bool CSend(const char *buf, int size)
  {
     pthread_mutex_lock(&c_lock);
     bool b = Send(control_fd, buf, size);
     pthread_mutex_unlock(&c_lock);
     return b;
  } 

  //! Given a memory pointer and a size, send a message using the control socket
  bool CSend(char* buf, int size)
  {
     bool b = CSend((const char *)buf, size);
     return b;
  }

  //! Given a string, send a message using the control socket
  bool CSend(std::string s)
  {
     pthread_mutex_lock(&c_lock);
     bool b = Send(control_fd, s);
     pthread_mutex_unlock(&c_lock);
     return b;
  }

  //! Given a set of references to pointers to memory and a set of references to sizes, receive a multi-part message using the conrol socket
  bool CRecv(char*& buf, int& size)
  {
     pthread_mutex_lock(&c_lock);
     bool b = Recv(control_fd, buf, size);
     pthread_mutex_unlock(&c_lock);
     return b;
  }

  //! Given a reference to a string, receive a message using the control socket
  bool CRecv(std::string& s)
  {
     pthread_mutex_lock(&c_lock);
     bool b = Recv(control_fd, s);
     pthread_mutex_unlock(&c_lock);
     return b;
  }

  //! Wait for the control socket to be unlocked
  //! \param sec max time to wait
  bool CWait(float sec) { bool b = Wait(control_fd, sec); return b; }

  //! Atomically send a message and receive a reply using the control socket
  bool CSendRecv(std::string& s)
  {
    pthread_mutex_lock(&c_lock);
    bool b = Send(control_fd, s);
    if (b)
      b = Recv(control_fd, s);
    pthread_mutex_unlock(&c_lock);
    return b;
  }

  //! Given a set of memory pointers and sizes, send them as a contiguous multi-part message using the data socket
  bool DSendV(char** buf, int* size)
  {
     pthread_mutex_lock(&d_lock);
     bool b = SendV(data_fd, buf, size);
     pthread_mutex_unlock(&d_lock);
     return b;
  }

  //! Given a memory pointer and a size, send a message using the data socket
  bool DSend(const char *buf, int size)
  {
     pthread_mutex_lock(&d_lock);
     bool b = Send(data_fd, buf, size);
     pthread_mutex_unlock(&d_lock);
     return b;
  } 

  //! Given a memory pointer and a size, send a message using the data socket
  bool DSend(char* buf, int size)
  {
     bool b = CSend((const char *)buf, size);
     return b;
  }

  //! Given a memory pointer and a size, send a message using the data socket
  bool DSend(std::string s)
  {
     pthread_mutex_lock(&d_lock);
     bool b = Send(data_fd, s);
     pthread_mutex_unlock(&d_lock);
     return b;
  }

  //! Given a set of references to pointers to memory and a set of references to sizes, receive a multi-part message using the data socket
  bool DRecv(char*& buf, int& size)
  {
     pthread_mutex_lock(&d_lock);
     bool b = Recv(data_fd, buf, size);
     pthread_mutex_unlock(&d_lock);
     return b;
  }

  //! Given a reference to a string, receive a message using the data socket
  bool DRecv(std::string& s)
  {
     pthread_mutex_lock(&d_lock);
     bool b = Recv(data_fd, s);
     pthread_mutex_unlock(&d_lock);
     return b;
  }

  //! Atomically send a message and receive a reply usint the data socket
  bool DSendRecv(std::string& s)
  {
    pthread_mutex_lock(&d_lock);
    bool b = Send(data_fd, s);
    if (b)
      b = Recv(data_fd, s);
    pthread_mutex_unlock(&d_lock);
    return b;
  }

  //! Wait for the data socket to be unlocked
  //! \param sec max time to wait
  bool DWait(float sec) { bool b = Wait(data_fd, sec); return b; }
  
private:
  bool SendV(int fd, char** buf, int* size);
  bool Send(int fd, const char *buf, int size);
  bool Send(int fd, char* buf, int size) { return Send(fd, (const char *)buf, size); }
  bool Send(int fd, std::string s) { return Send(fd, s.c_str(), s.size()+1); }
  bool Recv(int fd, char*& buf, int& size);
  bool Recv(int fd, std::string& s)
  {
    char *b; int n;
    if (! Recv(fd, b, n))
      return false;
    s = b;
    free(b);
    return true;
  }
  bool Wait(int fd, float sec);

  int connect_fd(struct sockaddr*);

  int data_fd;
  int control_fd;
  pthread_mutex_t c_lock;
  pthread_mutex_t d_lock;
};

}
