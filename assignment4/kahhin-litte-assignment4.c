/******************************************************************************
 CSci 4061 Fall 2017
 Assignment# 4
 Name: <Kah Hin Lai>
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>

#define NAMESIZE 256
#define DEBUG 1

pthread_mutex_t  lock;

typedef struct arguments
{
    int* total_size;
    char* file_path;
}arg;
/******************************************************************************
                                  Function
*******************************************************************************/

void * get_total_size(void* argu){
  int array_size = 10;
  pthread_t *threads=calloc(array_size, sizeof(pthread_t) );
  int numofthreads = 0;
  int n;
  arg t_argu;
  t_argu.file_path = ((arg*) argu) -> file_path;
  t_argu.total_size = ((arg*) argu) -> total_size;
  DIR * dpntr = opendir(t_argu.file_path);
  struct dirent *dentry;
  while( (dentry = readdir (dpntr)) != 0 ) {
    if(!strcmp(dentry->d_name,".") || !strcmp(dentry->d_name,"..")){
      continue;
		}
		else{
			struct stat statdata;
      char filepathname[NAMESIZE];
			sprintf(filepathname, "%s/%s",((arg*) argu) -> file_path, dentry->d_name);
			stat (filepathname,&statdata);
			if(S_ISDIR(statdata.st_mode)){
        t_argu.file_path = filepathname;
        pthread_t t;
        if ( (n = pthread_create(&t, NULL, get_total_size , &t_argu))) {
           fprintf(stderr,"pthread_create :%s\n",strerror(n));
           exit(1);
        }
        *(threads + numofthreads) = t;
        numofthreads++;
        if(array_size<numofthreads){
          threads = calloc (5, sizeof(pthread_t));
          array_size += 5;
        }
			}
			else{
        pthread_mutex_lock ( &lock);
        if(DEBUG){
          printf("DEBUG: %s %lld\n",filepathname, statdata.st_size);
        }
				*t_argu.total_size += statdata.st_size;
        pthread_mutex_unlock ( &lock);
			}
    }
  }
  if(numofthreads>0){
    for(int i = 0; i < numofthreads ;i++){
        pthread_join(*(threads+i), NULL);
    }
  }
  return 0;
}


/******************************************************************************
                                    MAIN
*******************************************************************************/
int main(int argc, char *argv[])
{
  int n;
  int total = 0;
  if ( (n = pthread_mutex_init( &lock, NULL))){
    fprintf(stderr,"mutex_init :%s\n",strerror(n));
    exit(1);
  }
	char *input_dir_name;
  pthread_t t;
	input_dir_name = (char *) malloc(NAMESIZE * sizeof(char));
	printf("Enter a directory name in the current directory: ");
	scanf("%s", input_dir_name);
  // Check for existence of directory
  DIR * temp;
  if( (temp = opendir(input_dir_name)) == 0){
    fprintf(stderr, "Error in opening directory: %s ", input_dir_name);
    perror("input_dir_namemkdir");
    exit(0);
  } else {
    closedir(temp);
  }
  //----------------------------------
  arg pass_arg;
  pass_arg.total_size = &total;
  pass_arg.file_path = input_dir_name;
  if ( (n = pthread_create(&t, NULL, get_total_size ,&pass_arg))) {
     fprintf(stderr,"pthread_create :%s\n",strerror(n));
     exit(1);
  }
  pthread_join(t, NULL);
  printf("total_size in dir(%s): %d\n",input_dir_name,total );
  return 0;
}
