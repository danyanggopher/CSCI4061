/***********************************************************************************************

 CSci 4061 Fall 2017
 Assignment# 3: Piper program for executing pipe commands

 Student name: Danyang Wang   Kah Hin Lai
 Student ID:   5197204        5329250

 Student name: <full name of second student>
 Student ID:   <Second student's ID>

 X500 id: wang6132, laixx330

 Operating system on which you tested your code: Linux
 CSELABS machine: <machine you tested on eg: xyz.cselabs.umn.edu>

 GROUP INSTRUCTION:  Please make only ONLY one  submission when working in a group.
***********************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define DEBUG 1

#define MAX_INPUT_LINE_LENGTH 2048 // Maximum length of the input pipeline command
                                   // such as "ls -l | sort -d +4 | cat "
#define MAX_CMDS_NUM   8           // maximum number of commands in a pipe list
                                   // In the above pipeline we have 3 commands
#define MAX_CMD_LENGTH 4096         // A command has no more than 4098  characters

FILE *logfp;

int num_cmds = 0;
char *cmds[MAX_CMDS_NUM];
int cmd_pids[MAX_CMDS_NUM];
int cmd_status[MAX_CMDS_NUM];

/* shared variables for descriptors */
int fildes[2], pipe2[2];

/*******************************************************************************/
/*   The function parse_command_line will take a string such as
     ls -l | sort -d +4 | cat | wc
     given in the char array commandLine, and it will separate out each pipe
     command and store pointer to the command strings in array "cmds"
     For example:
     cmds[0]  will pooint to string "ls -l"
     cmds[1] will point to string "sort -d +4"
     cmds[2] will point to string "cat"
     cmds[3] will point to string "wc"

     This function will write to the LOGFILE above information.
*/
/*******************************************************************************/

int parse_command_line (char commandLine[MAX_INPUT_LINE_LENGTH], char* cmds[MAX_CMDS_NUM]){
   logfp = fopen("LOGFILE", "a");

   int i = 0;
   int j = 0;
   char c;
   while (i < strlen(commandLine)) {
     char **acc;
     acc = malloc(MAX_INPUT_LINE_LENGTH * 1);
     acc[0] = malloc(sizeof(char) * 256);
     int l = 0;
     while (((c = commandLine[i]) != '|') && (i < strlen(commandLine))) {
       //printf("c is : %c \n", c);
       acc[0][l] = c;
       l += 1;
       i += 1;
     }
     i += 1;
     /****try to strip acc[0] here****/
     while(isspace((unsigned char)*(acc[0]))) {
       acc[0]++;
     }
     char *tail;
     tail = acc[0] + strlen(acc[0]) - 1;
     while(tail > acc[0] && isspace((unsigned char)*tail)) {
       tail--;
     }
     *(tail+1) = 0;
     /****try to strip acc[0] here****/
     cmds[j] = acc[0];
     j += 1;
   }
   int count = 0;
   for (int k = 0; k < (int)sizeof(cmds);k++){
     if (cmds[k] != NULL){
       /****write to the LOGFILE****/
       fprintf(logfp, "Command %d info: %s\n", k, cmds[k]);
       count++;
       //printf("cmds[%d] is : %s \n", k, cmds[k]);
     }
   }
   fprintf(logfp, "Number of commands from input: %d\n\n\n", count);
   fflush(logfp);
   return count;
}

/*******************************************************************************/
/*  parse_command takes command such as
    sort -d +4
    It parses a string such as above and puts command program name "sort" in
    argument array "cmd" and puts pointers to ll argument string to argvector
    It will return  argvector as follows
    command will be "sort"
    argvector[0] will be "sort"
    argvector[1] will be "-d"
    argvector[2] will be "+4"
/
/*******************************************************************************/

void parse_command(char input[MAX_CMD_LENGTH],
                   char command[MAX_CMD_LENGTH],
                   char *argvector[MAX_CMD_LENGTH]){
     int i = 0;
     char* delim = " \t\n\0";

     char temp[MAX_CMD_LENGTH];
     strcpy(temp, input);
     argvector[i] = strtok(temp, delim);
     strcpy(command, argvector[i]);
     i++;
     while ((argvector[i] = strtok(NULL, delim)) != NULL){
       i++;
     }
}


/*******************************************************************************/
/*  The function print_info will print to the LOGFILE information about all    */
/*  processes  currently executing in the pipeline                             */
/*  This printing should be enabled/disabled with a DEBUG flag                 */
/*******************************************************************************/

void print_info(char* cmds[MAX_CMDS_NUM],
		int cmd_pids[MAX_CMDS_NUM],
		int cmd_stat[MAX_CMDS_NUM],
		int num_cmds) {
    if (DEBUG) {
      //logfp = fopen("LOGFILE", "a");
      for (int i = 0; i < num_cmds; i++){
        fprintf(logfp, "PID: %d      Command: %s      Status: %d\n", cmd_pids[i], cmds[i], cmd_stat[i]);
      }
      fflush(logfp);
    }
}



/*******************************************************************************/
/*     The create_command_process  function will create a child process        */
/*     for the i'th command                                                    */
/*     The list of all pipe commands in the array "cmds"                       */
/*     the argument cmd_pids contains PID of all preceding command             */
/*     processes in the pipleine.  This function will add at the               */
/*     i'th index the PID of the new child process.                            */
/*     Following ADDED on  10/27/2017                                          */
/*     This function  will  craete pipe object, execute fork call, and give   */
/*     appropriate directives to child process for pipe set up and             */
/*     command execution using exec call                                       */
/*******************************************************************************/


void create_command_process (char cmds[MAX_CMD_LENGTH],   // Command line to be processed
                     int cmd_pids[MAX_CMDS_NUM],          // PIDs of preceding pipeline processes
                                                          // Insert PID of new command processs
		             int i)                               // commmand line number being processed
{
    // call parse_command inside this function
    // call fork inside this function

    char just_cmd[MAX_CMD_LENGTH];
    char *argvector[MAX_CMD_LENGTH];

    parse_command(cmds, just_cmd, argvector);

    if (cmd_pids[i] = fork()) { // parent process

      logfp = fopen("LOGFILE", "a");
      fprintf(logfp, "Command: %s with PID: %d is created\n", cmds, cmd_pids[i]);
      fflush(logfp);

      close(fildes[0]);
      close(fildes[1]);
    } else {                    // child process
      if (fildes[0] != -1) {
        dup2(fildes[0], STDIN_FILENO);
      }
      if (pipe2[1] != -1) {
        dup2(pipe2[1], STDOUT_FILENO);
      }
      close(fildes[0]);
      close(fildes[1]);
      close(pipe2[0]);
      close(pipe2[1]);

      if (execvp(just_cmd, argvector)){
        exit(1);
      }
    }

    fildes[0] = pipe2[0];
    fildes[1] = pipe2[1];

    pipe2[0] = -1;
    pipe2[1] = -1;
}


/********************************************************************************/
/*   The function waitPipelineTermination waits for all of the pipeline         */
/*   processes to terminate.                                                    */
/********************************************************************************/

void waitPipelineTermination () {
     int num_finished = 0;
     int cid_finished = 0;
     int terminate_status = 0;
     int i;
     int interrupted = 0;
     int fatalError = 0;

     while (num_finished < num_cmds) {
       if ((cid_finished = wait(&terminate_status)) == -1){
         // error : fail to wait
         perror("Wait Terminated:");
         interrupted = 1;
         break;
       }

       if (interrupted) {
         continue;
       }
       // go through the cmds and check pid of the process being terminated
       for (i = 0; i < num_cmds; i ++){
         if (cmd_pids[i] == cid_finished) {
           cmd_status[i] = terminate_status;

           //logfp = fopen("LOGFILE", "a");
           fprintf(logfp, "waiting... Process %d finished\n", cmd_pids[i]);
           fprintf(logfp, "Process id %d finished with exit status %d\n", cmd_pids[i], cmd_status[i]);
           fflush(logfp);

           num_finished++;
         }
       }

       if (WEXITSTATUS(terminate_status) != 0) {  // not exit normally
         fatalError = 1;
         printf("Terminating pipeline because process %d failed to execute\n", cid_finished);
         break;
       }
     }

     if (fatalError) {
       for (i = 0; i < num_cmds; i++){
         printf("Terminating process %d \n", cmd_pids[i]);
         kill(cmd_pids[i], 9);
       }
     }

}

/********************************************************************************/
/*  This is the signal handler function. It should be called on CNTRL-C signal  */
/*  if any pipeline of processes currently exists.  It will kill all processes  */
/*  in the pipeline, and the piper program will go back to the beginning of the */
/*  control loop, asking for the next pipe command input.                       */
/********************************************************************************/

void killPipeline( int signum ) {
    printf("\n Ctrl-C reveived, now kill all processes in the pipeline\n");
    for (int i = 0; i < num_cmds; i++) {
      kill(cmd_pids[i], SIGKILL);
    }

}

/********************************************************************************/

int main(int ac, char *av[]){

  int i,  pipcount;
  //check usage
  if (ac > 1){
    printf("\nIncorrect use of parameters\n");
    printf("USAGE: %s \n", av[0]);
    exit(1);
  }

  /* Set up signal handler for CNTRL-C to kill only the pipeline processes  */

  logfp =  fopen("LOGFILE", "a");


  while (1) {
     signal(SIGINT, SIG_DFL );
     pipcount = 0;

     /*  Get input command file anme form the user */
     char pipeCommand[MAX_INPUT_LINE_LENGTH];

     fflush(stdout);
     printf("Give a list of pipe commands: ");
     gets(pipeCommand);
     char* terminator = "quit";
     printf("You entered : list of pipe commands  %s\n", pipeCommand);
     if ( strcmp(pipeCommand, terminator) == 0  ) {
        fflush(logfp);
        fclose(logfp);
        printf("Goodbye!\n");
        exit(0);
     }
    /* Empty the cmds */
    for (int k = 0; k < (int)sizeof(cmds);k++){
      cmds[k] = NULL;
    }

    num_cmds = parse_command_line( pipeCommand, cmds);
    printf("\n num_cmds is %d\n", num_cmds);
    /*  SET UP SIGNAL HANDLER  TO HANDLE CNTRL-C                         */
    signal(SIGINT, killPipeline);

    /*  num_cmds indicates the number of commands in the pipeline        */

    /* The following code will create a pipeline of processes, one for   */
    /* each command in the given pipe                                    */
    /* For example: for command "ls -l | grep ^d | wc -l "  it will      */
    /* create 3 processes; one to execute "ls -l", second for "grep ^d"  */
    /* and the third for executing "wc -l"                               */
    fildes[0] = -1;
    fildes[1] = -1;
    pipe2[0] = -1;
    pipe2[1] = -1;

    for(i=0;i<num_cmds;i++){
         /*  CREATE A NEW PROCCES EXECUTTE THE i'TH COMMAND    */
         /*  YOU WILL NEED TO CREATE A PIPE, AND CONNECT THIS NEW  */
         /*  PROCESS'S stdin AND stdout  TO APPROPRIATE PIPES    */
         printf("Command %d info: %s\n", i, cmds[i]);
         if (pipcount < num_cmds - 1) { // do not have to create pipe for the last Command
            pipe(pipe2);
            pipcount++;
          }
          create_command_process (cmds[i], cmd_pids, i);
    }
    fprintf(logfp, "\n\n");
    waitPipelineTermination();
    fprintf(logfp, "\n\n");
    print_info(cmds, cmd_pids, cmd_status, num_cmds);
    fprintf(logfp, "\n\n\n\n");
    fflush(logfp);
  }
} //end main

/*************************************************/
