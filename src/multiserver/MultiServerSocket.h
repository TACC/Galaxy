#pragma once

#include <pthread.h>
#include <string>

namespace gxy
{

class MultiServerSocket
{
public:
  MultiServerSocket(int cfd, int dfd) : data_fd(dfd) , control_fd(cfd) {}
  MultiServerSocket(const char* host, int port);
  ~MultiServerSocket();
  
  bool CSendV(char** buf, int* size)    { pthread_mutex_lock(&c_lock); bool b = SendV(control_fd, buf, size);  pthread_mutex_unlock(&c_lock); return b; }
  bool CSend(const char *buf, int size) { pthread_mutex_lock(&c_lock); bool b = Send(control_fd, buf, size);  pthread_mutex_unlock(&c_lock); return b; } 
  bool CSend(char* buf, int size)       { pthread_mutex_lock(&c_lock); bool b = CSend((const char *)buf, size);  pthread_mutex_unlock(&c_lock); return b; }
  bool CSend(std::string s)             { pthread_mutex_lock(&c_lock); bool b = Send(control_fd, s);  pthread_mutex_unlock(&c_lock); return b; }
  bool CRecv(char*& buf, int& size)     { pthread_mutex_lock(&c_lock); bool b = Recv(control_fd, buf, size);  pthread_mutex_unlock(&c_lock); return b; }
  bool CRecv(std::string& s)            { pthread_mutex_lock(&c_lock); bool b = Recv(control_fd, s);  pthread_mutex_unlock(&c_lock); return b; }
  bool CWait(float sec)                 { bool b = Wait(control_fd, sec); return b; }

  bool CSendRecv(std::string& s)
  {
    pthread_mutex_lock(&c_lock);
    bool b = Send(control_fd, s);
    if (b)
      b = Recv(control_fd, s);
    pthread_mutex_unlock(&c_lock);
    return b;
  }
  
  bool DSendV(char** buf, int* size)    { pthread_mutex_lock(&d_lock); bool b = SendV(data_fd, buf, size);  pthread_mutex_unlock(&d_lock); return b; }
  bool DSend(const char *buf, int size) { pthread_mutex_lock(&d_lock); bool b = Send(data_fd, buf, size);  pthread_mutex_unlock(&d_lock); return b; } 
  bool DSend(char* buf, int size)       { pthread_mutex_lock(&d_lock); bool b = DSend((const char *)buf, size); pthread_mutex_unlock(&d_lock); return b; }
  bool DSend(std::string s)             { pthread_mutex_lock(&d_lock); bool b = Send(data_fd, s); pthread_mutex_unlock(&d_lock); return b; }
  bool DRecv(char*& buf, int& size)     { pthread_mutex_lock(&d_lock); bool b = Recv(data_fd, buf, size); pthread_mutex_unlock(&d_lock); return b; }
  bool DRecv(std::string& s)            { pthread_mutex_lock(&d_lock); bool b = Recv(data_fd, s); pthread_mutex_unlock(&d_lock); return b; }
  bool DWait(float sec)                 { bool b = Wait(data_fd, sec); return b; }

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
