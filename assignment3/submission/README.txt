The program take a command line as input, which is separated by "|".
Then the program creates a child process for each command in the pipe,
the current child will read input from the previous child through pipes
we created. After all child processes get created and executed, the parent
will wait for all its child processes to terminate. The signaction of SIGINT is replaced
by a function called "killPipeline", which will kill all the child processes when some child in the pipe is running.
All the information will be printed in the file "LOGFILE", including the pid and exit status of each child process
