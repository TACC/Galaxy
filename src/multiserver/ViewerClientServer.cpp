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

#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

// #include <ospray/ospray.h>

#include <string.h>

#include "rapidjson/document.h"

using namespace rapidjson;

#include "ViewerClientServer.h"

using namespace gxy;

namespace gxy
{

extern "C" void
init()
{
  ViewerClientServer::init();
}

extern "C" ViewerClientServer *
new_handler(SocketHandler *sh)
{
  return new ViewerClientServer(sh);
}


void
ViewerClientServer::init()
{
  Renderer::Initialize();
  ServerRendering::RegisterClass();
}

bool
ViewerClientServer::handle(string line, string& reply)
{
  DatasetsDPtr theDatasets = Datasets::DCast(MultiServer::Get()->GetGlobal("global datasets"));
  if (! theDatasets)
  {
    theDatasets = Datasets::NewDistributed();
    MultiServer::Get()->SetGlobal("global datasets", theDatasets);
  }

  std::stringstream ss(line);
  std::string cmd;

  ss >> cmd;

  if (cmd == "permute_pixels")
  {
    std::string onoff;
    ss >> onoff;
    if (ss.fail() || onoff == "on")
    {
      GetTheRenderer()->SetPermutePixels(true);
      reply = "ok";
      return true;
    }
    else if (onoff == "off")
    {
      GetTheRenderer()->SetPermutePixels(false);
      reply = "ok";
      return true;
    }
    else
    {
      reply = "error non-default arg to permute_pixels must be on or off";
      return true;
    }
  }

  else if (cmd == "max_rays_per_packet")
  {
    int n;
    ss >> n;
    if (ss.fail())
    {
      reply = "max_rays_per_packet requires integer arg";
      return true;
    }
    else
    {
      GetTheRenderer()->SetMaxRayListSize(n);
      reply = "ok";
      return true;
    }
  }

  else if (cmd == "commit")
  {
    Commit();
    reply = "ok";
    return true;
  }

  else if (cmd == "json")
  {
    Document doc;

    string json, tmp;
    while (std::getline(ss, tmp))
      json = json + tmp;

    if (doc.Parse<0>(json.c_str()).HasParseError())
    {
      reply = "error parsing json command";
      return true;
    }
  
    if (doc.HasMember("Datasets"))
    {
      if (!GetTheDatasets()->LoadFromJSON(doc))
      {
        reply = "error loading datasets from json command";
        return true;
      }

      if (! GetTheDatasets()->Commit())
      {
        reply = "error committing datasets from json command";
        return true;
      }
      reply = "ok";
      return true;
    }

    if (doc.HasMember("Visualization"))
    {
      GetTheVisualization()->Clear();

      if (!GetTheVisualization()->LoadFromJSON(doc["Visualization"]))
      {
        reply = "error loading visualization from json command";
        return true;
      }

      if (!GetTheVisualization()->Commit(GetTheDatasets()))
      {
        reply = "error committing visualization from json command";
        return true;
      }
    }

    if (doc.HasMember("Camera"))
    {
      if (!GetTheCamera()->LoadFromJSON(doc["Camera"]))
      {
        reply = "error loading camera from json command";
        return true;
      }

      if (!GetTheCamera()->Commit())
      {
        reply = "error committing camera from json command";
        return true;
      }
    }

    if (doc.HasMember("Renderer"))
    {
      if (!GetTheRenderer()->LoadStateFromDocument(doc))
      {
        reply = "error loading renderer from json command";
        return true;
      }

      if (!GetTheRenderer()->Commit())
      {
        reply = "error committing renderer from json command";
        return true;
      }
    }

    reply = "ok";
    return true;
  }
  else if (cmd == "window")
  {
    int w, h;
    ss >> w >> h;
    if (ss.fail())
    {
      reply = "error window command needs width and height as integers";
      return true;
    }

    // DHR: not clear what to do here ...
    GetTheCamera()->set_width(w);
    GetTheCamera()->set_height(h);
    GetTheCamera()->Commit();

    // assuming the Rendering already has the camera ...
    GetTheRendering()->SetTheSize(w, h);
    GetTheRendering()->SetHandler(this);
    GetTheRendering()->Commit();

    reply = "ok";
    return true;
  }
  else if (cmd == "render")
  {
    // Commit();
    render();
    reply = "ok";
    return true;
  }
  else if (cmd == "renderMany")
  {
    // Commit();
    int n;
    ss >> n;
    for (auto i = 0; i < n; i++)
    {
      render();
      struct timespec t;
      t.tv_sec  = 0;
      t.tv_nsec = 100000000;
      nanosleep(&t, NULL);
    }
    reply = "ok";
    return true;
  }
  else
    return false;
}

}
