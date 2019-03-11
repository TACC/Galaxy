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

#include <iostream>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>

#include "CommandLine.h"

namespace gxy
{

static void brk() { std::cerr << "break\n"; }

bool
CommandLine::handle_command(std::string line, SocketHandler *skt)
{
  std::stringstream ss(line);
  std::string cmd;

  ss >> cmd;

  // Remove leading whitespace
  cmd = cmd.erase(0, cmd.find_first_not_of(" \t\n\r\f\v"));

  // If its zero length ignore it
  if (cmd.size() == 0) return false;

  // If its quit, then quit
  else if (cmd == "quit") return true;

  // If its break, then break
  else if (cmd == "break") brk();

  // If its 'file', open file and process its contents
  else if (cmd == "file")
  {
    std::string filename;
    ss >> filename;

    std::ifstream ifs(filename);
    if (ifs)
      CommandLine::handle_command_stream((std::istream*)&ifs, skt);
    else
      std::cerr << "unable to open " << filename << "\n";
  }
 
  // Otherwise, if there's a server attached, send it
  else if (skt)
  {
    if (! skt->CSendRecv(line))
    {
      std::cerr << "send/receive failed\n";
      exit(1);
    }

    std::stringstream ss(line);

    std::string op, rest;
    ss >> op;
    std::getline(ss, rest);
    if (op == "error")
      std::cerr << "error: " << rest << "\n";
    else if (op == "ok")
    {
      if (rest != "") std::cerr << rest << "\n";
      while (std::getline(ss, rest))
        if (rest != "") std::cerr << rest << "\n";
    }
  }

  else
    std::cerr << "unrecognized command: " << line;

  return false;
}

void
CommandLine::handle_command_stream(std::istream *is, SocketHandler *skt)
{
  std::string cmd = "", tmp; bool done = false;
  std::cerr << "? ";
  while (!done && std::getline(*is, tmp))
  {
    for (size_t n = tmp.find(";"); ! done && n != std::string::npos; n = tmp.find(";"))
    {
      std::string prefix = tmp.substr(0, n);
      if (cmd == "") cmd = prefix; else cmd = cmd + " " + prefix;
      done = handle_command(cmd, skt);
      cmd = "";
      tmp = tmp.substr(n+1);
    }
    if (! done)
    {
      if (cmd == "") cmd = tmp; else cmd = cmd + " " + tmp;
    }
    std::cerr << "? ";
  }
  if (cmd != "")
    handle_command(cmd, skt);
}

static void *
command_thread(void *d)
{
  SocketHandler *skt = (SocketHandler *)d;

  CommandLine::handle_command_stream((std::istream *)&std::cin, skt);

  exit(1);
}

void
CommandLine::StartCommandLineThread(SocketHandler *skt)
{
  pthread_t tid;
  pthread_create(&tid, NULL, command_thread, (void *)skt);
}

void 
CommandLine::Run(std::string filename, SocketHandler *skt)
{
  if (filename == "-")
    handle_command_stream((std::istream*)&std::cin, skt);
  else
  {
    std::ifstream ifn(filename);
    if (ifn)
      CommandLine::handle_command_stream((std::istream*)&ifn, skt);
    else
      std::cerr << "unable to open " << filename << "\n";
  }
}

void 
CommandLine::Run(SocketHandler *skt)
{
  handle_command_stream((std::istream*)&std::cin, skt);
}

}
