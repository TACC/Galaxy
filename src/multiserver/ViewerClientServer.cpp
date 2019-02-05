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

#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

#include <ospray/ospray.h>

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

extern "C" MultiServerHandler *
new_handler(DynamicLibraryP dlp, int cfd, int dfd)
{
  return new ViewerClientServer(dlp, cfd, dfd);
}


void
ViewerClientServer::init()
{
  Renderer::Initialize();
  ServerRendering::RegisterClass();
}

string
ViewerClientServer::handle(string line)
{
  DatasetsP theDatasets = Datasets::Cast(MultiServer::Get()->GetGlobal("global datasets"));
  if (! theDatasets)
  {
    theDatasets = Datasets::NewP();
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
      return string("ok");
    }
    else if (onoff == "off")
    {
      GetTheRenderer()->SetPermutePixels(false);
      return string("ok");
    }
    else
      return string("error non-default arg to permute_pixels must be on or off");
  }

  else if (cmd == "max_rays_per_packet")
  {
    int n;
    ss >> n;
    if (ss.fail())
      return string("max_rays_per_packet requires integer arg");
    else
    {
      GetTheRenderer()->SetMaxRayListSize(n);
      return string("ok");
    }
  }

  else if (cmd == "commit")
  {
    Commit();
    return string("ok");
  }

  else if (cmd == "json")
  {
    Document doc;

    string json, tmp;
    while (std::getline(ss, tmp))
      json = json + tmp;

    if (doc.Parse<0>(json.c_str()).HasParseError())
      return string("error parsing json command");
  
    if (doc.HasMember("Datasets"))
    {
      if (!GetTheDatasets()->LoadFromJSON(doc))
        return string("error loading datasets from json command");

      if (! GetTheDatasets()->Commit())
        return string("error committing datasets from json command");
    }

    if (doc.HasMember("Visualization"))
    {
      if (!GetTheVisualization()->LoadFromJSON(doc["Visualization"]))
        return string("error loading visualization from json command");

      if (!GetTheVisualization()->Commit(GetTheDatasets()))
        return string("error committing visualization from json command");
    }

    if (doc.HasMember("Camera"))
    {
      if (!GetTheCamera()->LoadFromJSON(doc["Camera"]))
        return string("error loading camera from json command");

      if (!GetTheCamera()->Commit())
        return string("error committing camera from json command");
    }

    if (doc.HasMember("Renderer"))
    {
      if (!GetTheRenderer()->LoadStateFromDocument(doc))
        return string("error loading renderer from json command");

      if (!GetTheRenderer()->Commit())
        return string("error committing renderer from json command");
    }

    return string("ok");
  }
  else if (cmd == "window")
  {
    int w, h;
    ss >> w >> h;
    if (ss.fail())
      return string("error window command needs width and height as integers");

    GetTheRendering()->SetTheSize(w, h);
    GetTheRendering()->SetHandler(this);
    GetTheRendering()->Commit();

    return string("ok");
  }
  else if (cmd == "render")
  {
    render();
    return string("ok");
  }
  else
    return MultiServerHandler::handle(line);
}

}
