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