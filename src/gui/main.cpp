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

#include <nodes/NodeData>
#include <nodes/FlowScene>
#include <nodes/FlowView>
#include <nodes/ConnectionStyle>
#include <nodes/TypeConverter>

#include <QApplication>
#include <QSurfaceFormat>

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QMenuBar>

#include <nodes/DataModelRegistry>

#include "GxyMainWindow.hpp"
#include "GxyConnectionMgr.hpp"

using namespace std;

int mpiRank = 0, mpiSize = 1;
#include "Debug.h"

GxyConnectionMgr *_theGxyConnectionMgr;
GxyConnectionMgr *getTheGxyConnectionMgr() { return  _theGxyConnectionMgr; }

static void
setStyle()
{
  ConnectionStyle::setConnectionStyle(
  R"(
  {
    "ConnectionStyle": {
      "ConstructionColor": "gray",
      "NormalColor": "black",
      "SelectedColor": "gray",
      "SelectedHaloColor": "deepskyblue",
      "HoveredColor": "deepskyblue",

      "LineWidth": 3.0,
      "ConstructionLineWidth": 2.0,
      "PointDiameter": 10.0,

      "UseDataDefinedColors": true
    }
  }
  )");
}

void
syntax(char *a)
{
  std::cerr << "syntax " << a << " [options]\n";
  std::cerr << "options:\n";
  std::cerr << "  -D           debug\n";
  std::cerr << "  -s server    default server (localhost)\n";
  std::cerr << "  -p port      default port (5001)\n";
  std::cerr << "  -S           show conversation\n";
  std::cerr << "  -c           connect on startup\n";
  exit(1);
}

int
main(int argc, char *argv[])
{
  _theGxyConnectionMgr = new GxyConnectionMgr();
  getTheGxyConnectionMgr()->addModule("libgxy_module_gui.so");

  QApplication app(argc, argv);
  GxyMainWindow mainWindow;

  bool startit = false;
  bool dbg = false;
  bool show = false;
  for (int i = 1; i < argc; i++)
    if (! strcmp(argv[i], "-s")) getTheGxyConnectionMgr()->setServer(argv[++i]); 
    else if (! strcmp(argv[i], "-p")) getTheGxyConnectionMgr()->setPort(argv[++i]); 
    else if (! strcmp(argv[i], "-D")) dbg = true;
    else if (! strcmp(argv[i], "-S")) getTheGxyConnectionMgr()->showConversation(true); 
    else if (! strcmp(argv[i], "-c")) startit = true;
    else syntax(argv[0]);

  Debug *d = dbg ? new Debug(argv[0], false, "") : NULL;

  if (startit)
    getTheGxyConnectionMgr()->connectToServer();

  setStyle();

  app.exec();
}
