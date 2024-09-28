#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <cstring>

using namespace std;

int main() {

  // Both ends of the pipe.
  int pipefds[2];

  // Create pipe from array. (0 means it worked)
  int status = pipe(pipefds);

  // If something went wrong with the pipe.
  if (status != 0)
    {
      cerr << "pipe() call failed!" << endl;
      return 1;
    }

  int pid = fork();

  if (pid != 0) // parent
    {
      close(pipefds[1]); // Not using this part of the pipe.
      dup2(pipefds[0], fileno(stdin));
      
      cout << "do wc (parent)" << endl;

      int myLength;
      string wc = "wc";
      myLength = wc.length();
      char *args[2];
      args[0] = new char[myLength + 1];
      strcpy(args[0], wc.c_str());
      args[1] = NULL;
      execvp(args[0], args);
    }
  else
    {

      close(pipefds[0]); // Not using this part of the pipe.
      dup2(pipefds[1], fileno(stdout));
      
      cout << "do ls -l (child)" << endl;

      int myLength;
      string str = "ls";
      myLength = str.length();
      char *args[3]; // One for ls, one for -l, one for NULL.
      args[0] = new char[myLength + 1];
      strcpy(args[0], str.c_str());

      str = "-l";
      myLength - str.length();
      args[1] = new char[myLength + 1];
      strcpy(args[1], str.c_str());
      execvp(args[0], args);
    }
  
  return 0;
}
