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
 *  using three connections: one for control, one for data and a third for
 *  asynchronous events. A locking mechanism is used so that multi-part messages
 *  and send/receive pairs are
 */

#pragma once

#include <pthread.h>
#include <unistd.h>
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
  //! \param efd file descriptor for events socket
  SocketHandler(int cfd, int dfd, int efd);

  ~SocketHandler();

  //! Attempt to open a connection to a remote server - return false when unable to connect
  //! \param host name of host to connect to
  //! \param port port to connect to
  virtual bool Connect(std::string host, int port);
  virtual bool Connect(char * host, int port);

  //! Given a set of memory pointers and sizes, send them as a contiguous multi-part message using the control socket
  bool CSendV(char** buf, int* size)
  {
     pthread_mutex_lock(&locks[1]);
     bool b = SendV(fds[1], buf, size);
     pthread_mutex_unlock(&locks[1]);
     return b;
  }

  //! Given a memory pointer and a size, send a message using the control socket
  bool CSend(const char *buf, int size)
  {
     pthread_mutex_lock(&locks[1]);
     bool b = Send(fds[1], buf, size);
     pthread_mutex_unlock(&locks[1]);
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
     pthread_mutex_lock(&locks[1]);
     bool b = Send(fds[1], s);
     pthread_mutex_unlock(&locks[1]);
     return b;
  }

  //! Given a set of references to pointers to memory and a set of references to sizes, receive a multi-part message using the control socket
  bool CRecv(char*& buf, int& size)
  {
     pthread_mutex_lock(&locks[1]);
     bool b = Recv(fds[1], buf, size);
     pthread_mutex_unlock(&locks[1]);
     return b;
  }

  //! Given a reference to a string, receive a message using the control socket
  bool CRecv(std::string& s)
  {
     pthread_mutex_lock(&locks[1]);
     bool b = Recv(fds[1], s);
     pthread_mutex_unlock(&locks[1]);
     return b;
  }

  //! Wait for the control socket to be unlocked
  //! \param sec max time to wait
  bool CWait(float sec) { bool b = Wait(fds[1], sec); return b; }

  //! Atomically send a message and receive a reply using the control socket
  bool CSendRecv(std::string& s)
  {
    pthread_mutex_lock(&locks[1]);
    bool b = Send(fds[1], s);
    if (b)
      b = Recv(fds[1], s);
    pthread_mutex_unlock(&locks[1]);
    return b;
  }

  //! Given a set of memory pointers and sizes, send them as a contiguous multi-part message using the data socket
  bool DSendV(char** buf, int* size)
  {
     pthread_mutex_lock(&locks[0]);
     bool b = SendV(fds[0], buf, size);
     pthread_mutex_unlock(&locks[0]);
     return b;
  }

  //! Given a memory pointer and a size, send a message using the data socket
  bool DSend(const char *buf, int size)
  {
     pthread_mutex_lock(&locks[0]);
     bool b = Send(fds[0], buf, size);
     pthread_mutex_unlock(&locks[0]);
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
     pthread_mutex_lock(&locks[0]);
     bool b = Send(fds[0], s);
     pthread_mutex_unlock(&locks[0]);
     return b;
  }

  //! Given a set of references to pointers to memory and a set of references to sizes, receive a multi-part message using the data socket
  bool DRecv(char*& buf, int& size)
  {
     pthread_mutex_lock(&locks[0]);
     bool b = Recv(fds[0], buf, size);
     pthread_mutex_unlock(&locks[0]);
     return b;
  }

  //! Given a reference to a string, receive a message using the data socket
  bool DRecv(std::string& s)
  {
     pthread_mutex_lock(&locks[0]);
     bool b = Recv(fds[0], s);
     pthread_mutex_unlock(&locks[0]);
     return b;
  }

  //! Atomically send a message and receive a reply usint the data socket
  bool DSendRecv(std::string& s)
  {
    pthread_mutex_lock(&locks[0]);
    bool b = Send(fds[0], s);
    if (b)
      b = Recv(fds[0], s);
    pthread_mutex_unlock(&locks[0]);
    return b;
  }

  //! Wait for the data socket to be unlocked
  //! \param sec max time to wait
  bool DWait(float sec) { bool b = Wait(fds[0], sec); return b; }

  //! Given a set of memory pointers and sizes, send them as a contiguous multi-part message using the event socket
  bool ESendV(char** buf, int* size)
  {
     pthread_mutex_lock(&locks[2]);
     bool b = SendV(fds[2], buf, size);
     pthread_mutex_unlock(&locks[2]);
     return b;
  }

  //! Given a memory pointer and a size, send a message using the event socket
  bool ESend(const char *buf, int size)
  {
     pthread_mutex_lock(&locks[2]);
     bool b = Send(fds[2], buf, size);
     pthread_mutex_unlock(&locks[2]);
     return b;
  } 

  //! Given a memory pointer and a size, send a message using the event socket
  bool ESend(char* buf, int size)
  {
     bool b = ESend((const char *)buf, size);
     return b;
  }

  //! Given a string, send a message using the event socket
  bool ESend(std::string s)
  {
     pthread_mutex_lock(&locks[2]);
     bool b = Send(fds[2], s);
     pthread_mutex_unlock(&locks[2]);
     return b;
  }

  //! Given a set of references to pointers to memory and a set of references to sizes, receive a multi-part message using the event socket
  bool ERecv(char*& buf, int& size)
  {
     pthread_mutex_lock(&locks[2]);
     bool b = Recv(fds[2], buf, size);
     pthread_mutex_unlock(&locks[2]);
     return b;
  }

  //! Given a reference to a string, receive a message using the event socket
  bool ERecv(std::string& s)
  {
     pthread_mutex_lock(&locks[2]);
     bool b = Recv(fds[2], s);
     pthread_mutex_unlock(&locks[2]);
     return b;
  }

  //! Wait for the event socket to be unlocked
  //! \param sec max time to wait
  bool EWait(float sec) { bool b = Wait(fds[2], sec); return b; }

  //! Atomically send a message and receive a reply using the event socket
  bool ESendRecv(std::string& s)
  {
    pthread_mutex_lock(&locks[2]);
    bool b = Send(fds[2], s);
    if (b)
      b = Recv(fds[2], s);
    pthread_mutex_unlock(&locks[2]);
    return b;
  }

  //! Default event handler
  virtual void EventHandler(gxy::SocketHandler *);

  //! Disconnect
  void Disconnect();

  //! test whether connection exists
  bool IsConnected() { return is_connected; }

  void showConversation(bool yn) { show = yn; }
  
  bool event_thread_quit = false;

private:
  bool show = false;
  bool is_connected = false;

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

  int fds[3];
  pthread_mutex_t locks[3];
  pthread_t event_tid = (pthread_t)-1;
};

}
