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
  int id;
  sharedv *ptr;

  int key = atoi(argv[0]);

  id = shmget(key, 0, 0);
  if (id == -1) {
    perror("consumer shmget failed");
    exit(1);
  }

  ptr = shmat(id, (void *) NULL, 1023);
  if (ptr == (void *) -1) {
    perror ("child shmat failed");
    exit(2);
  }

  FILE* cons_log;
  char log_name[128];
  sprintf(log_name, "%s.txt", "CONSUMER");
  cons_log = fopen(log_name, "a");

  int i;
  for (i = 0; i < 3000; i++) {
    pthread_mutex_lock(&(ptr->lock));

    while ((ptr->count) == 0) {
      while (pthread_cond_wait(&(ptr->ItemAvailable), &(ptr->lock)) != 0);
    }
    if ((ptr->count) > 0) {
      char item[128];
      strcpy(item, ptr->Buffer[ptr->tail]);
      printf("Deposit item %s\n", item);
      fprintf(cons_log, "Deposit: %s\n", ptr->Buffer[ptr->tail]);
      ptr->tail = ((ptr->tail) + 1) % (ptr->N);
      ptr->count = ptr->count - 1;
    }

    pthread_mutex_unlock(&(ptr->lock));

    pthread_cond_signal(&(ptr->SpaceAvailable));
  }

  shmdt( (void *) ptr);
}
