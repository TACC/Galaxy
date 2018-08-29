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

/*! \file Timer.h 
 * \brief provides a high-resolution timer for performance analysis in Galaxy
 * \ingroup framework
 */

#include <chrono>
#include <ctime>
#include <ratio>
#include <string>

namespace gxy
{

//! provides a high-resolution timer for performance analysis in Galaxy
/*! \ingroup framework 
 * The timer records time using the high_resolution_clock and supports multiple calls to 
 * \ref start and \ref stop. 
 * 
 * When the Timer
 * object is destroyed, it records the timing results to file with the
 * name `gxytimer_<name>-<rank>.out` where `<name>` is the string given
 * when the timer is constructed, and `<rank>` is the MPI rank of the process
 * where the timer was located.
 */  
class Timer
{
public:
  //! construct a timer with the given name
  Timer(std::string f) : basename(f)
  {
    tot = 0;
    first = true;
  }

  //! write out the results of this timer to file and destroy the Timer object
  ~Timer()
  {
    std::stringstream ss;
    ss << "gxytimer_" << basename << "-" << rank << ".out";
    std::ofstream log;
    log.open (ss.str());
    log << tot << " seconds\n";
    log.close();
  }

  //! start this timer
  void start()
  {
    if (first)
    {
      first = false;
      rank = GetTheApplication()->GetRank();
    }
    t0 = std::chrono::high_resolution_clock::now();
  }

  //! stop this timer
  void stop()
  {
    std::chrono::duration<double> d = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - t0);
    tot += d.count();
  }

private:
  bool first;
  int rank;
  std::chrono::high_resolution_clock::time_point t0;
  double tot;
  std::string basename;
};

} // namespace gxy
