/*******************************************************************
 * quote_server.c --- Server takes the name of the configuration
 * file as an argument. it will go in a loop listening for client
 * session requests on a specified port. Upon receiving a session
 * connection requests on port 6789, the server should create a
 * thread that will handle that client session.
 *******************************************************************/

/* includes */
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TRUE 1
#define SERVER_PORT 5001
#define BUFFER_SIZE 1024


void die(const char *);
void pdie(const char *);

struct config_and_sock {
  int msgsock_;
  char config_[BUFFER_SIZE];
};

void * select_quote(void *arg) {
  int rval;

  char req[BUFFER_SIZE];
  char response[BUFFER_SIZE];

    /* The function pointer which handles the client session */
    /* Open all the quote files in read mode */
    struct config_and_sock* cas = (struct config_and_sock*) arg;

    FILE *config_file_d;
    FILE *computer_file;
    FILE *einstein_file;
    FILE *twain_file;

    /* cannot hard-code */
    config_file_d = fopen(cas->config_, "r");


    computer_file = fopen("computers.txt", "r");
    einstein_file = fopen("einstein.txt", "r");
    twain_file = fopen("twain.txt", "r");

    int finished = 0;

    while (!finished) {
      if ((rval = recv(cas->msgsock_, &req, sizeof(req), 0)) < 0) {
        pdie("Reading stream message");
      }

      if (rval == 0) {
        /* Client has closed the connection */
        fprintf(stderr, "Server: Ending connection\n");
      }

      char buf[BUFFER_SIZE];
      char buf_next[BUFFER_SIZE];

      if (strcmp(req, "BYE") == 0) {
        finished = 1;
      } else if (strcmp(req, "GET: QUOTE CAT: ANY\n") == 0) {
        /* get a random quote from random file */
      } else if (strcmp(req, "GET: QUOTE CAT: Computers\n") == 0) {
        fgets(buf, BUFFER_SIZE, computer_file);
        fgets(buf_next, BUFFER_SIZE, computer_file);
      } else if (strcmp(req, "GET: QUOTE CAT: Einstein\n") == 0) {
        fgets(buf, BUFFER_SIZE, einstein_file);
        fgets(buf_next, BUFFER_SIZE, einstein_file);
      } else if (strcmp(req, "GET: QUOTE CAT: Twain\n") == 0) {
        fgets(buf, BUFFER_SIZE, twain_file);
        fgets(buf_next, BUFFER_SIZE, twain_file);
      } else {
        printf("invalid request\n");
      }

      /* send response */
      sprintf(response, "%s%s", buf, buf_next);
      if (send(cas->msgsock_, &response, sizeof(response), 0) < 0) {
        pdie("Writing on stream socket");
      }
    }

}

int main(int argc, char *argv[]) {

  int sock;
  int msgsock;                      /* not sure whether it's needed */
  struct sockaddr_in server;
  struct sockaddr_in client;
  int clientLen;

  /* Open a socket */
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    pdie("Opening stream socket");
  }

  /* Fill out inetaddr struct */
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(SERVER_PORT);

  /* Bind */
  if (bind(sock, (struct sockaddr *) &server, sizeof(server))) {
    pdie("Binding stream socket");
  }

  printf("Server: Socket has port %hu\n", ntohs(server.sin_port));

  /* Listen w/ max queue of 5 */
  listen(sock, 5);

  /* Loop, waiting for client connections. Create a thread to handle
   * the client session.
   */
  while (TRUE) {

    clientLen = sizeof(client);
    if ((msgsock = accept(sock, (struct sockaddr *) &client, &clientLen)) == -1) {
      pdie("Accept");
    } else {
      /* create detached thread and set PTHREAD_SCOPE_SYSTEM */
      /* set pthread attributes */
      pthread_attr_t attr;
      pthread_t tid;
      int n;

      pthread_attr_init(&attr);
      pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
      /* create pthread, pass ((void *) req) as argument */
      struct config_and_sock* cs = malloc(sizeof(struct config_and_sock));
      cs->msgsock_ = msgsock;
      strcpy(cs->config_, argv[1]);
      if (n = pthread_create(&tid, &attr, select_quote, (void *) cs)) {
        fprintf(stderr, "pthread_create :%s\n",strerror(n));
        exit(1);
      }
      pthread_attr_destroy( &attr );
    }
  }
}

void pdie(const char *mesg) {

  perror(mesg);
  exit(1);
}

void die(const char *mesg) {

  fputs(mesg, stderr);
  fputc('\n', stderr);
  exit(1);
}
