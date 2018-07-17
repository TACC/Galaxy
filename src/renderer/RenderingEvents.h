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

#include "Events.h"

#include <iostream>

namespace gxy
{
class StartRenderingEvent : public Event
{
public:
 StartRenderingEvent() {}

protected:
  void print(std::ostream& o)
  {
    Event::print(o);
    o << "StartRendering";
  }
};

class CameraLoopEndEvent : public Event
{
public:
 CameraLoopEndEvent() {}

protected:
  void print(std::ostream& o)
  {
    Event::print(o);
    o << "CameraLoopEnd";
  }
};

class RayListEvent : public Event
{
public:
 RayListEvent(RayList *rl)
	{
		n  = rl->GetRayCount();
		id = rl->GetId();
		r  = rl->GetTheRendering()->getkey();
		rs = rl->GetTheRenderingSet()->getkey();
		f  = rl->GetFrame();
	}

protected:
  int n;
  int id;
	Key r;
	Key rs;
	int f;
};

class ProcessRayListEvent : public RayListEvent
{
public:
  ProcessRayListEvent(RayList *rl) : RayListEvent(rl) {}

protected:
  void print(std::ostream& o)
  {
    Event::print(o);
    o << "ProcessRayList:  id " << id << " n " << n << " r " << r << " rs " << rs << " f " << f;
  }
};

class SecondariesGeneratedEvent : public RayListEvent
{
public:
  SecondariesGeneratedEvent(RayList *rl) : RayListEvent(rl) {}

protected:
  void print(std::ostream& o)
  {
    Event::print(o);
    o << "SecondariesGenerated:  id " << id << " n " << n << " r " << r << " rs " << rs << " f " << f;
  }
};

class PixelsEvent : public Event
{
public: 
	PixelsEvent(int n, Key rk, int f) : n(n), rk(rk), f(f) {}

protected:
	int n;
	Key rk;
	int f;
};

class LocalPixelsEvent : public PixelsEvent
{
public:
	LocalPixelsEvent(int n, Key rk, int f) : PixelsEvent(n, rk, f) {}

protected:
  void print(std::ostream& o)
  {
    Event::print(o);
    o << "LocalPixelsEvent: n " << n << " r " << rk << " f " << f;
  }
};

class SendPixelsEvent : public PixelsEvent
{
public: 
	SendPixelsEvent(int n, Key rk, int f, int d) : dst(d), PixelsEvent(n, rk, f) {}

protected:
	int dst;

  void print(std::ostream& o)
  {
    Event::print(o);
    o << "SendPixelsEvent: n " << n << " r " << rk << " f " << f << " dst " << dst;
  }
};

class RcvPixelsEvent : public PixelsEvent
{
public: 
	RcvPixelsEvent(int n, Key rk, int f, int s) : src(s), PixelsEvent(n, rk, f) {}

protected:
	int src;

  void print(std::ostream& o)
  {
    Event::print(o);
    o << "RcvPixelsEvent: n " << n << " r " << rk << " f " << f << " src " << src;
  }
};

#if 0
class TraceRaysEvent : public Event
{
public:
  TraceRaysEvent(int n, int p, int s, Key r) : nin(n), phits(p), secondaries(s), rset(r) {};

protected:
  void print(ostream& o)
  {
    Event::print(o);
    o << "TraceRays processed " << nin << " rays with " << phits << " hits generating " << secondaries << " secondaries for set " << rset;
  }

  private:
    int nin;
    int phits;
    int secondaries;
    Key rset;
};

#endif

class ProcessRayListCompletedEvent : public Event
{
public:
  ProcessRayListCompletedEvent(int ni, int nn, int nr, int ns) : nIn(ni), nSecondaries(nn), nRetired(nr), nSent(ns) {}

protected:
  void print(std::ostream& o)
  {
    Event::print(o);
    o << "ProcessRayListCompleted:  " << nIn << " in, " << nSecondaries << " spawned, " << nRetired << " retired and " << nSent << " sent";
  }

private:
  int nIn, nSecondaries, nSent, nRetired;
};

#if 0

class LocalPixelContributionsEvent : public Event
{
public:
  LocalPixelContributionsEvent(int c, RenderingSetP r) : count(c), rset(r->getkey()) { }

protected:
  void print(ostream& o)
  { 
    Event::print(o);
    o << count << " pixels handled locally for rset " << rset;
  }

private:
  int count;
  Key rset;
};

class RemotePixelContributionsEvent : public Event
{
public:
  RemotePixelContributionsEvent(int c, int owner, RenderingSetP rs) : count(c), dest(owner),  rset(rs->getkey()) {}

protected:
  void print(ostream& o)
  {
    Event::print(o);
    o << count << " pixels sent to " << dest << " for rset " << rset;
  }

private:
  int count;
  int dest;
  Key rset;
};

#endif
}

