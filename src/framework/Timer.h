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

#include <ctime>
#include <ratio>
#include <chrono>

namespace gxy
{
class Timer
{
public:
  Timer(string f) : basename(f)
  {
    tot = 0;
    first = true;
  }

  ~Timer()
  {
    stringstream ss;
    ss << basename << "-" << rank << ".out";
    ofstream log;
    log.open (ss.str());
    log << tot << " seconds\n";
    log.close();
  }

  void start()
  {
    if (first)
    {
      first = false;
      rank = GetTheApplication()->GetRank();
    }
    t0 = high_resolution_clock::now();
  }

  void stop()
  {
    duration<double> d = duration_cast<duration<double>>(high_resolution_clock::now() - t0);
    tot += d.count();
  }

private:
  bool first;
  int rank;
  high_resolution_clock::time_point t0;
  double tot;
  string basename;
};
}