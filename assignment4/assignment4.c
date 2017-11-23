/* This program will sum the size of all regular files in a directory by
recursively visiting all nested directories. The program will ask the user
to specify a directory pathname and then it will compute the sum of
the size of all the regular files inside that directory and its subdirectories.
*/

/*****Include*****/
#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

/* global variables */
int sum = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/* The function pointer which will recursively create threads
   number of threads needed in one call:
    # of subdirectories
    The thread handling a directory will wait for the completion of the child threads that it created to
    handle the nested subdirectories
*/
void * sumDirectory(void *arg){
    /* calculate the total size of the current directory */
    struct stat statbuf;
    DIR *dp;
    struct dirent *direntry;

    /* create an array to store all child_thread */
    /*
    pthread_t* childs = malloc(256 * sizeof(pthread_t));
    int index_childs = -1;
    */
    int tempsum = 0;
    dp = opendir((char*) arg);
    while((direntry = readdir(dp)) != NULL) {
      /* construct the fullName */
      char fullName [256];
      snprintf(fullName, sizeof(fullName), "%s/%s", (char*) arg, direntry->d_name);
      stat(fullName,&statbuf);
      if (!(S_ISDIR(statbuf.st_mode))) {
        /* not a directory, count the file size */
        /* need a critical section to increment the sum */
        pthread_mutex_lock(&lock);
        int fs = (int) statbuf.st_size;
        tempsum += fs;
        sum += fs;
        pthread_mutex_unlock(&lock);
      } else {
        /* It's a directory, have to make recursive call */
        char subdir[256];
        if (strcmp(direntry->d_name, ".") == 0 || strcmp(direntry->d_name, "..") == 0){
          continue;
        }
        snprintf(subdir, sizeof(subdir), "%s/%s", (char*) arg, direntry->d_name);
        /* have to create new threads for each subdirectories */
        pthread_t child_thread;
        int n;
        if (n = pthread_create(&child_thread, NULL, sumDirectory, (void *) subdir)) {
          fprintf(stderr, "pthread_create :%s\n",strerror(n));
          exit(1);
        }

        void *subdir_size;
        /*
        pthread_mutex_lock(&lock);
        index_childs++;
        childs[index_childs] = child_thread;
        pthread_mutex_unlock(&lock);
        */
        /* After making a recursive call,
          have to join the child threads
        */
        if (n = pthread_join(child_thread, &subdir_size)){
          fprintf(stderr, "pthread_join: %s\n", strerror(n));
          exit(1);
        }
        tempsum += (int)subdir_size;
      }
    }
    /*int m;
    //printf("index_childs is : %d\n", index_childs);
    for (int i = 0; i<=index_childs;i++){
      if (m = pthread_join(childs[i], NULL)){
        fprintf(stderr, "pthread_join: %s\n", strerror(m));
        exit(1);
      }
    }
    */
    /* Ater the while loop, print out the result of the current directory */
    printf("\n%s: %d\n", (char*) arg, tempsum);
    return (void *)tempsum;

}

/* Main function will ask user to specify a pathname and then it will compute
the total size of the directory tree*/
int main(int argc, char* argv[]){
    /* ask for input */
    char* input_dir_name;
    input_dir_name = (char *) malloc(256 * sizeof(char));
    printf("ENTER A RELATIVE PATH: \n");
    scanf("%s", input_dir_name);

    /* convert input to the actual path name */
    char actualpath [PATH_MAX];
	  char *ptr;
	  ptr = realpath(input_dir_name, actualpath);

    /* create a thread to execute sunDirectory, initalize the process */
    pthread_t main_thread;
    int n;

    /* create a thread with default attributes */
    if (n = pthread_create(&main_thread, NULL, sumDirectory, (void *) actualpath)){
      fprintf(stderr,"pthread_create :%s\n",strerror(n));
      exit(1);
    }

    if (n = pthread_join(main_thread, NULL)){
      fprintf(stderr, "pthread_join: %s\n", strerror(n));
  		exit(1);
    }

    /* after executing the main_thread, we should have correct sum as the result */
    printf("the total size is: %d\n", sum);
}

/* not sure about the use of pthread_cond_wait
   do we need make some thread detached ?
*/
