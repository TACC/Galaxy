#pragma once

#include <map>
#include <string>

#include "GxyConnectionMgr.hpp"

class GxyRenderWindow;

class GxyRenderWindowMgr
{
public:
  GxyRenderWindowMgr();
  ~GxyRenderWindowMgr();

  void RegisterRenderWindow(std::string id, GxyRenderWindow* w);
  GxyRenderWindow * GetRenderWindow(std::string id);
  void Remove(std::string id);
  void StartReceiving();
  void StopReceiving();

private:
  static void *pixel_receiver_thread(void *d);
  pthread_t rcvr_tid = NULL;
  bool kill_threads = false;

  std::map<std::string, GxyRenderWindow *> window_map;
};

GxyRenderWindowMgr *getTheGxyRenderWindowMgr();


