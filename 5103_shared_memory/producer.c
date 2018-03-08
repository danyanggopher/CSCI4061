#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

typedef struct {
  int N;
  int head;
  int tail;
  int count;
  pthread_mutex_t lock;
  pthread_cond_t SpaceAvailable;
  pthread_cond_t ItemAvailable;
  char Buffer[2][128];
} sharedv;

main(int argc, char* argv[]) {
  time_t rawtime;
  struct tm * timeinfo;
  FILE *prod_log;
  char log_name[128];

  int id;
  sharedv *ptr;

  int key = atoi(argv[0]);
  char* color = argv[1];

  sprintf(log_name, "%s_%s.txt", "PRODUCER", color);
  prod_log = fopen(log_name, "a");

  id = shmget(key, 0, 0);
  if (id == -1) {
    perror("producer shmget failed");
    exit(1);
  }

  ptr = shmat(id, (void *) NULL, 1023);
  if (ptr == (void *) -1) {
    perror ("child shmat failed");
    exit(2);
  }

  int i;
  for (i = 0; i < 1000; i++) {

    pthread_mutex_lock(&(ptr->lock));

    while ((ptr->count) == (ptr->N)) {
      while (pthread_cond_wait(&(ptr->SpaceAvailable), &(ptr->lock)) != 0);
    }
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    char item[128];
    strcpy(item, color);
    strcat(item, asctime(timeinfo));
    fprintf(prod_log, "Deposit %s\n", item);
    strcpy(ptr->Buffer[ptr->head], item);
    ptr->head = (ptr->head + 1) % (ptr->N);
    ptr->count = ptr->count + 1;

    pthread_mutex_unlock(&(ptr->lock));

    pthread_cond_signal(&(ptr->ItemAvailable));
  }

  shmdt( (void *) ptr);
}
