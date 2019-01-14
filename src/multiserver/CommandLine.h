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

/*! \file CommandLine.h
 *  \brief CommandLine handles a simple command-line thread processing thread 
 *  that knows about a CommandLine, across which it can pass command control.
 */

#pragma once

#include <pthread.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "SocketHandler.h"

namespace gxy
{

class CommandLine
{
public:
  CommandLine() {}

  //! Run an asynchronous command-line manager
  void StartCommandLineThread(SocketHandler *skt = NULL);

  //! Run synchronously
  void Run(char *filename, SocketHandler *skt = NULL);

  //! process a stream of commands, may be stdin or from a file
  static void handle_command_stream(std::istream *, SocketHandler *skt = NULL);

  //! handle processes each individual command
  static bool handle_command(std::string line, SocketHandler *skt = NULL);
};

}
