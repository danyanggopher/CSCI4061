/*
 fork four processes.
 create shared memory for buffer, locks,
 and other variables shared among the processes.
 wait for all the processes.
*/

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

#define DEBUG

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

main(int argc, char *argv[]) {
  int shmem_id;

  key_t key;
  int size;
  int flag;

  key = 4242;
  size = 2048;
  flag = 1023;

  shmem_id = shmget(key, size, flag);
  if (shmem_id == -1) {
    perror("shmget failed");
    exit(1);
  }
  #ifdef DEBUG
  printf("Got shmem id = %d\n", shmem_id);
  #endif

  /* put shared variables in the shared memory */
  int ret;
  pthread_mutexattr_t lockattr;
  pthread_condattr_t condattr;

  pthread_mutexattr_init(&lockattr);
  pthread_condattr_init(&condattr);
  pthread_mutexattr_setpshared(&lockattr, PTHREAD_PROCESS_SHARED);
  pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED);
  sharedv* sv = (sharedv *)malloc(2048);

  printf("shared data structure initalized\n");

  sv = shmat(shmem_id, (void *) NULL, 1023);
  if (sv == (void *) -1) {
    perror ("shmat failed");
    exit(2);
  }
  #ifdef DEBUG
  printf("Got ptr = %p\n", sv);
  #endif

  sv->N = 2;
  sv->head = 0;
  sv->tail = 0;
  sv->count = 0;
  pthread_mutex_init(&sv->lock, &lockattr);
  pthread_cond_init(&sv->SpaceAvailable, &condattr);
  pthread_cond_init(&sv->ItemAvailable, &condattr);
  // sv->Buffer = (char **)malloc(2 * sizeof(char *));

  /* create producer processes */
  char keystr[10];
  sprintf(keystr, "%d", key);
  if (fork()) {
    printf("producer red\n");
  } else {
    execl("./producer", keystr, "RED", NULL);
  }

  if (fork()) {
    printf("producer black\n");
  } else {
    execl("./producer", keystr, "BLACK", NULL);
  }

  if (fork()) {
    printf("producer white\n");
  } else {
    execl("./producer", keystr, "WHITE", NULL);
  }

  int cons_pid;
  if ((cons_pid = fork()) != 0) {
    printf("consumer\n");
  } else {
    execl("./consumer", keystr, NULL);
  }

  /* wait for the consumer process */
  int status;
  ret = waitpid(cons_pid, &status, NULL);

  shmctl (shmem_id, IPC_RMID, NULL);
}
