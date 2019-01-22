#include <iostream>
#include <fstream>

using namespace std;

bool
handle(string cmd)
{
  cmd = cmd.erase(0, cmd.find_first_not_of(" \t\n\r\f\v"));
  std::cerr << cmd << "\n";
  return cmd == "quit";
}

void
handle_commands(istream *is)
{
  string cmd = "", tmp; bool done = false;
  cerr << "? ";
  while (!done && std::getline(*is, tmp))
  {
    // FOO; quit; BAR;
    for (size_t n = tmp.find(";"); ! done && n != string::npos; n = tmp.find(";"))
    {
      string prefix = tmp.substr(0, n);
      if (cmd == "") cmd = prefix; else cmd = cmd + " " + prefix;
      done = handle(cmd);
      cmd = "";
      tmp = tmp.substr(n+1);
    }
    if (! done)
      if (cmd == "") cmd = tmp; else cmd = cmd + " " + tmp;
  }
  if (cmd != "")
    handle(cmd);
}


int
main(int argc, char **argv)
{
  handle_commands((istream *)&cin);

  ifstream ifn(argv[1]);
  handle_commands((istream *)&ifn);

#if 0
  istream *in = &cin;
  handle_commands(in);

  ifstream ifn(argv[1]);
  in = &ifn;
  handle_commands(in);
#endif
}
  

  
