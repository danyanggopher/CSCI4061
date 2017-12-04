

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BUFFER_SIZE 1024

#define PORT "6789"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold


struct QUOTE_FILE_WITH_TITLE {
  FILE * fd;
  char title[60];
  char filename[60];
};
typedef struct QUOTE_FILE_WITH_TITLE QUOTE_FILE;

struct socket_context
{
    int sock_fd;
    struct sockaddr_storage sock_addr;
    char* config_file;
};
typedef struct socket_context client_sock_context;

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

QUOTE_FILE* readfile(FILE * fp, int* num_catg) {
  int num = 0;
  if(fp == NULL) {
   perror("Error opening file");
   exit(-1);
  }
  char str[60];
  QUOTE_FILE * qf = malloc(2 * sizeof(QUOTE_FILE));
  for (int i = 0; fgets(str, 60, fp) != NULL ;i++) {
    if (i = num) {
      // qf = calloc(1, sizeof(QUOTE_FILE));
      qf = realloc(qf, (num+1) * sizeof(QUOTE_FILE));
      if (qf == NULL) {
        perror("fail realloc QUOTE_FILE");
        exit(-1);
      }
    }
    char filename[60];
    if(sscanf(str, "%s %s ",qf[i].title , qf[i].filename) != 2){
      perror("wrong format configuration");
      exit(-1);
    }
    qf[i].title[strlen(qf[i].title)-1] = 0;
    qf[i].fd = fopen(qf[i].filename, "r");
    num++;
    // printf("%s: %s\n",qf[i].title, qf[i].filename );
  }
  *(num_catg) = num;
  fclose(fp);
  return qf;
}

void random_initialization (void){
  time_t t;
  srand((unsigned) time(&t));
}

int decode(char * str, QUOTE_FILE* qf, int num_catg) {
  for(int i=0; i< num_catg ;i++) {
    if(strcmp(str, qf[i].title) == 0) {
      return i;
    }
  }
  return -1;
}

char * get_quote (int i, QUOTE_FILE* qf) {
  char* str1 = calloc(BUFFER_SIZE, sizeof(char));
  if(fgets(str1, BUFFER_SIZE, qf[i].fd) != NULL) {
    char* str2 = calloc(100, sizeof(char));
    fgets(str2, BUFFER_SIZE, qf[i].fd);
    return strcat(str1,str2);
  } else {
    qf[i].fd = fopen(qf[i].filename, "r");
    return get_quote(i,qf);
  }
}

int pickquote(char* buffer, QUOTE_FILE* qf, char* response, int num_catg) {
  char str[BUFFER_SIZE];
  sscanf(buffer,"GET: QUOTE CAT: %s\n",str);
  if (strcmp(str, "ANY") == 0) {
    sprintf(response,"%s\n",get_quote(rand()%num_catg, qf));
    // printf("%s\n",response );
    return 1;
  } else {
    int i = decode(str, qf, num_catg);
    if(i == -1){
      return 0;
    } else {
      sprintf(response,"%s\n",get_quote(i, qf));
      return 1;
    }
  }
}

void *handle_request(void * arg){
    client_sock_context *client_sock;
    client_sock =(client_sock_context*)arg;
    char s[INET6_ADDRSTRLEN];
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int rval, num_catg;
    random_initialization();
    inet_ntop(client_sock->sock_addr.ss_family,
        get_in_addr((struct sockaddr *)&client_sock->sock_addr),
        s, sizeof s);
    QUOTE_FILE * qfs = readfile(fopen(client_sock->config_file, "r"), &num_catg);
    printf("%d: got connection from %s\n", pthread_self(),s);
    while(1){
      if ((rval = recv(client_sock->sock_fd, &buffer, sizeof(buffer),0)) < 0){
        perror("Reading stream message");
        exit(-1);
      }
      if (strcmp(buffer, "BYE") == 0) {
        break;
      } else if (strcmp(buffer, "GET: LIST\n") == 0) {
        sprintf(response,"%s\n",qfs[0].title);
        for(int i=1; i< num_catg ;i++){
          sprintf(response,"%s%s\n",response,qfs[i].title);
        }
      } else if (!pickquote(buffer,qfs,response, num_catg)) {
        sprintf(response,"Invalid Commands\n");
      }
      if (send(client_sock->sock_fd, response, sizeof(response), 0) == -1) {
          perror("send");
      }
    }
    close(client_sock->sock_fd);
    free(client_sock);
    pthread_exit(0);
}

int main(int argc, char **argv)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    int rv;
    fd_set rfds;
    struct timeval tv;
    int retval;

    pthread_t th;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
	FD_ZERO(&rfds);
	FD_SET(sockfd,&rfds);
	tv.tv_sec=5;
	tv.tv_usec=0;

	retval = select(sockfd+1,&rfds,NULL,NULL,&tv);

	if(retval){
		printf("New connection...\n");
	        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

    		if (new_fd == -1) {
        	    perror("accept");
            	    continue;
        	}

		client_sock_context* client_sock = (client_sock_context*) malloc(sizeof(client_sock_context));
		client_sock->sock_fd = new_fd;
		client_sock->sock_addr = their_addr;
    client_sock->config_file = argv[1];

		//create a new thread to handle this request
		//logic for child is present in handle_request() function defined above
		pthread_create(&th,NULL,handle_request,client_sock);
    	}
    	else{
		printf("No connection request...timeout..set timer again..\n");
	}
    }
    return 0;
}
