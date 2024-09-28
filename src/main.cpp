#include <iostream>
#include <csignal>
#include <cstring>
#include <unordered_map>
#include <unistd.h>
#include <sys/wait.h>

#include "Command.hpp"

// Construct a global umap containing process ID's and commands as key-value pairs.
unordered_map<int, Command> processUMap;

void backgroundedProcessHandler(int sigCode)
{
  // Get the pid of the completed child process.
  int pid = waitpid(0, NULL, WNOHANG); 
  if (pid > 0)
    {
      // Get the command using the pid as the key.
      cout << "Completed: PID = " << pid
	   << " : " << processUMap[pid] 
	   << endl << flush;

      // Remove the process from the umap.
      processUMap.erase(pid); 
    }
}

void executeCommand(Command &com, int prevPipe[2] = 0)
{
  // Construct args c-style array from com.args().
  char **args = new char *[com.numArgs() + 1];
  args[com.numArgs()] = NULL;
  for (int i = 0; i < com.numArgs(); i++)
    {
      args[i] = new char[com.args()[i].length() + 1];
      strcpy(args[i], com.args()[i].c_str());
    }

  // Change directory (check if command name is "cd").
  if (com.name() == "cd")
    {
      chdir(args[1]);
    }

  // All other processes must be forked!
  // Create the pipe that will provide input to the next process (If we pipe out).
  int nextPipe[2];
  pipe(nextPipe);
  int cpid = fork();

  // Child process (Not the shell!)
  if (!cpid)
    {
      // If the previous process piped out, the current process will take the prev's output as its input.
      if (prevPipe)
	{
	  close(prevPipe[1]);
	  dup2(prevPipe[0], fileno(stdin));
	}

      // If the current process is piping out, output to the pipe used for the next process's input.
      if (com.pipeOut())
	{
	  close(nextPipe[0]);
	  dup2(nextPipe[1], fileno(stdout));
	}

      // Close the pipe otherwise.
      else {close(nextPipe[1]);}

      // Redirect I/O to the specified file(s).
      if (com.redirIn())
	{
	  FILE *inFile = fopen(com.inputRedirectFile().c_str(), "r");
	  dup2(fileno(inFile), fileno(stdin));
	}

      if (com.redirOut())
	{
	  FILE *outFile = fopen(com.outputRedirectFile().c_str(), "w");
	  dup2(fileno(outFile), fileno(stdout));
	}

      // Finally, run the command.
      execvp(args[0], args);
    }

  
  // Shell process.
  // Free the memory allocated for the c-style array.
  for (int i = 0; i < com.numArgs(); i++)
    delete[] args[i];
  delete[] args;
 
  // If there's a previous pipe, close it.
  if (prevPipe)
    {
      close(prevPipe[0]);
      close(prevPipe[1]);
    }

  // If the command isn't backgrounded, wait for the child to finish.
  if (!com.backgrounded())
    {
      waitpid(cpid, NULL, 0);
    }
  // If the command IS backgrounded, add its child process ID and command to the umap
  // and signal when the child process has finished.
  else
    {
      processUMap[cpid] = com;
      signal(SIGCHLD, backgroundedProcessHandler);
    }

  // If the current command is piping out, read in the next command and execute it,
  // providing the current command's output pipe as the next command's input pipe.
  if (com.pipeOut())
    {
      com.read();
      executeCommand(com, nextPipe);
      return;
    }

  // Otherwise, close the output pipes and continue to the next command.
  else
    {
      close(nextPipe[0]);
      close(nextPipe[1]);
    }

  // Print command prompt and read in the next command.
  cout << ">>>> ";
  com.read();
}

int main(void)
{
    Command com;
    
    // Keep track of number for commands. 
    int num = 1; 
    
    // Prompt for and read in first command. 
    cout << ">>>> ";
    com.read();
    
    
    while(com.name() != "exit")
    {
      // Print out current command	
      cout << num++ << ")" << com << endl;

      // Execute current command and read in next command.
      executeCommand(com);
      //cout << endl;
    }

    // Done using the shell at this point.
    cout << "Thank you for using\n"
	 << " ^、\n"
	 << "(˚ˎ 。7   <-- His ass is NOT running a shell program.\n"
	 << "|、˜ \\\n"
	 << "じしˍ,)ノ\n"
	 << "shell. We now return you to your regularly scheduled shell!" << endl;
}
