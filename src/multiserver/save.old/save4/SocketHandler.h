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

#include <pthread.h>
#include <string>

namespace gxy
{

class SocketHandler
{
public:
  SocketHandler();
  SocketHandler(int cfd, int dfd);
  SocketHandler(const char* host, int port);

  ~SocketHandler();

  bool CSendV(char** buf, int* size)
  {
     pthread_mutex_lock(&c_lock);
     bool b = SendV(control_fd, buf, size);
     pthread_mutex_unlock(&c_lock);
     return b;
  }

  bool CSend(const char *buf, int size)
  {
     pthread_mutex_lock(&c_lock);
     bool b = Send(control_fd, buf, size);
     pthread_mutex_unlock(&c_lock);
     return b;
  } 

  bool CSend(char* buf, int size)
  {
     pthread_mutex_lock(&c_lock);
     bool b = CSend((const char *)buf, size);
     pthread_mutex_unlock(&c_lock);
     return b;
  }

  bool CSend(std::string s)
  {
     pthread_mutex_lock(&c_lock);
     bool b = Send(control_fd, s);
     pthread_mutex_unlock(&c_lock);
     return b;
  }

  bool CRecv(char*& buf, int& size)
  {
     pthread_mutex_lock(&c_lock);
     bool b = Recv(control_fd, buf, size);
     pthread_mutex_unlock(&c_lock);
     return b;
  }

  bool CRecv(std::string& s)
  {
     pthread_mutex_lock(&c_lock);
     bool b = Recv(control_fd, s);
     pthread_mutex_unlock(&c_lock);
     return b;
  }

  bool CWait(float sec) { bool b = Wait(control_fd, sec); return b; }

  bool CSendRecv(std::string& s)
  {
    pthread_mutex_lock(&c_lock);
    bool b = Send(control_fd, s);
    if (b)
      b = Recv(control_fd, s);
    pthread_mutex_unlock(&c_lock);
    return b;
  }

  bool DSendV(char** buf, int* size)
  {
     pthread_mutex_lock(&c_lock);
     bool b = SendV(control_fd, buf, size);
     pthread_mutex_unlock(&c_lock);
     return b;
  }

  bool DSend(const char *buf, int size)
  {
     pthread_mutex_lock(&c_lock);
     bool b = Send(control_fd, buf, size);
     pthread_mutex_unlock(&c_lock);
     return b;
  } 

  bool DSend(char* buf, int size)
  {
     pthread_mutex_lock(&c_lock);
     bool b = CSend((const char *)buf, size);
     pthread_mutex_unlock(&c_lock);
     return b;
  }

  bool DSend(std::string s)
  {
     pthread_mutex_lock(&c_lock);
     bool b = Send(control_fd, s);
     pthread_mutex_unlock(&c_lock);
     return b;
  }

  bool DRecv(char*& buf, int& size)
  {
     pthread_mutex_lock(&c_lock);
     bool b = Recv(control_fd, buf, size);
     pthread_mutex_unlock(&c_lock);
     return b;
  }

  bool DRecv(std::string& s)
  {
     pthread_mutex_lock(&c_lock);
     bool b = Recv(control_fd, s);
     pthread_mutex_unlock(&c_lock);
     return b;
  }

  bool DSendRecv(std::string& s)
  {
    pthread_mutex_lock(&d_lock);
    bool b = Send(data_fd, s);
    if (b)
      b = Recv(data_fd, s);
    pthread_mutex_unlock(&d_lock);
    return b;
  }
  
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
