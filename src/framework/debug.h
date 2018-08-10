#include <sys/stat.h>
#include <unistd.h>

class Debug
{
public:
  Debug(const char *executable, bool attach, char *arg)
  { 
    bool dbg = true;
    std::stringstream cmd;
    pid_t pid = getpid();

    bool do_me = true;
		bool first = true;
    if (*arg)
    { 
      int i; 
      for (i = 0; i < strlen(arg); i++)
        if (arg[i] == '-')
          break;
      
      if (i < strlen(arg))
      { 
        arg[i] = 0;
        int s = atoi(arg); 
        int e = atoi(arg + i + 1);
        do_me = (mpiRank >= s) && (mpiRank <= e);
				first = mpiRank == s;
      }
      else
        do_me = (mpiRank == atoi(arg));
    }

    if (do_me)
    { 
      if (attach)
        cerr << "Attach to PID " << pid << endl;
      else
      { 
				char scriptname[1024]; bool found = false;

				char *home = getenv("HOME");
				if (home)
				{
					sprintf(scriptname, "%s/dbg_script", home);
					struct stat buffer;   
					found = stat(scriptname, &buffer) == 0; 
				}

				if (! found)
				{
					char *galaxy = getenv("GALAXY_INSTALL");
					if (galaxy)
					{
						sprintf(scriptname, "%s/scripts/dbg_script", galaxy);
						struct stat buffer;   
						found = stat(scriptname, &buffer) == 0; 
					}
				}

				if (found)
				{
					cmd << "~/dbg_script " << executable << " " << pid << " &";
					system(cmd.str().c_str());
					while (dbg)
						sleep(1);
				}
				else if (first)
				{
					cerr << "no debug script found, can't debug\n";
					do_me = false;
				}
      }
    }
  }
};

