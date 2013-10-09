/**
 * Redistribution of this file is permitted under the GNU General
 * Public License v2.
 *
 * Copyright 2012 by Gabriel Parmer.
 * Author: Gabriel Parmer, gparmer@gwu.edu, 2012
 */
/**
 * Name: Brannon McGraw
 * email: bmcgraw@gwmail.gwu.edu
 * 
 *
 */

/* 
 * This is a HTTP server.  It accepts connections on port 8080, and
 * serves a local static document.
 *
 * The clients you can use are 
 * - httperf (e.g., httperf --port=8080),
 * - wget (e.g. wget localhost:8080 /), 
 * - or even your browser.  
 *
 * To measure the efficiency and concurrency of your server, use
 * httperf and explore its options using the manual pages (man
 * httperf) to see the maximum number of connections per second you
 * can maintain over, for example, a 10 second period.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#include <pthread.h>

#include <util.h> 		/* client_process */
#include <server.h>		/* server_accept and server_create */
#include "cas.h"

#define MAX_DATA_SZ 1024
#define MAX_CONCURRENCY 256

/* 
 * This is the function for handling a _single_ request.  Understand
 * what each of the steps in this function do, so that you can handle
 * _multiple_ requests.  Use this function as an _example_ of the
 * basic functionality.  As you increase the server in functionality,
 * you will want to probably keep all of the functions called in this
 * function, but define different code to use them.
 */

void
server_single_request(int accept_fd)
{
  int fd;

  /* 
   * The server thread will always want to be doing the accept.
   * That main thread will want to hand off the new fd to the
   * new threads/processes/thread pool.
   */
  fd = server_accept(accept_fd);
  client_process(fd);
	
  return;
}

void
server_multiple_requests(int accept_fd)
{
  int fd;
  while(19){
    
    fd = server_accept(accept_fd);
    client_process(fd);    
  }
  return;

}

void
server_processes(int accept_fd)
{
  int fd;
  int count = 0;
  while(19){
    
    if(count<MAX_CONCURRENCY){ //if there are enough resources for a new process
      printf("Listening for connection...\n");
      fd = server_accept(accept_fd); //wait on an incoming connection
      int pid; //create a new process ID
      count++; //increment the number of process IDs
      printf("Forking\n");
      pid = fork();
      
      if(pid == 0){ //if the child...
	printf("Processing request...\n");
	client_process(fd);
	printf("Closing connection...\n");
	exit(0);
      }else if(pid<0) { // if the child failed...
	printf("Unable to fork to child, exiting server.");
	return;
      }else{ //if the parent
	while( waitpid(-1, NULL, WNOHANG) > 0){
	  printf("%d\n",count);
	  count--;
	}
      }//end if (which process am i)
      
    } //end if (how many processes running)
    else{ //if too many connections,
      printf("Too many connections, flushing queue\n"); 
      while( waitpid(-1, NULL, 0) > 0){ 
	count--;
      }
    }
}//end while (infinite loops don't end though...)
  return;
}

void
server_dynamic(int accept_fd) 
{
  accept_fd *= 2;
  return;
}

void* pthread_handle(void* fd){
  printf("New thread handling the client\n");
  client_process((int)fd);
  printf("Thread exiting\n");
  pthread_exit(NULL);
  return (void*)1;
}


void
server_thread_per(int accept_fd)
{
  int i;
  int fd;
  int count;
  pthread_t thread [MAX_CONCURRENCY];
  while(19){
    if(count< MAX_CONCURRENCY){
      fd = server_accept(accept_fd);  
      if(pthread_create( &thread[count++], NULL, &pthread_handle, (void*)fd )){
	printf("Could not create thread.. exiting\n");
	count--;
      }/* end if (creating thread) */
    }else{ /* if too many threads open */
      
      printf("Waiting on threads to die\n");
      for(i = 0; i<count; i++){
	printf("Killing thread %d\n",i);
	if(pthread_join(thread[i], NULL)){
	  printf("Error joining thread\n");
	} /* end if (error in joining */
      }/*end for (clearing pthread queue) */
      count = 0;

    } /* end if (too many threads) */
  } /* end while (server execution) */

  return;
}



struct request{
  int fd;
  struct request* next;
};

struct request* requests;
volatile int lock = 0;

struct request* get_request(void){
  int old_lock, new_lock;
  struct request* r;
  do{
    old_lock = lock;
    new_lock = lock;
    new_lock++;
  }while(__cas( &lock, old_lock, new_lock)  );
  
  if(requests){
    r = requests;
    
    if(requests->next){
      requests = requests->next;
    }else{
      requests = NULL;
    }
    lock = 0;
    return r;
  }else{
    lock = 0;
    return NULL;
  }
}

void put_request(struct request* r){
  int old_lock, new_lock;
  
  do{
    old_lock = lock;
    new_lock = lock;
    new_lock++;
  }while(__cas(&lock, old_lock, new_lock));
  
  if(requests){
    r->next = requests;
  }else{
    r->next = NULL;
  }  
  requests = r;
  lock = 0;
}

void worker_handle(void){
  struct request* r;
  int fd;
  while(19){
    r = get_request();
    if(r){
      int old_lock, new_lock;
      
      do{
	old_lock = lock;
	new_lock = lock;
	new_lock++;
      }while(__cas(&lock2, old_lock, new_lock));
      
      fd = r->fd;
      client_process((int)fd);
      free(r);
    }
  }
  
}

void master_handle(void* accept_fd){
  pthread_t worker [MAX_CONCURRENCY];
  int i;
  requests = NULL;
  
  for(i=0; i<MAX_CONCURRENCY/4;i++){
    pthread_create(&worker[i], NULL, &worker_handle, (void*)NULL);
  }
  
  struct request* r;
  int fd;
  while(19){
    fd = server_accept((int)accept_fd);
    
    
    r = malloc(sizeof(struct request));
    r->fd = fd;
    r->next = NULL;
    
    put_request(r);
  }
  
}


void
server_task_queue(int accept_fd)
{
    pthread_t master;
    pthread_create(&master, NULL, &master_handle, (void*)accept_fd);
    while(19){
    
    }
}

void
server_thread_pool(int accept_fd)
{
  return;
}

typedef enum {
  SERVER_TYPE_ONE = 0,
  SERVER_TYPE_SINGLET,
  SERVER_TYPE_PROCESS,
  SERVER_TYPE_FORK_EXEC,
  SERVER_TYPE_SPAWN_THREAD,
  SERVER_TYPE_TASK_QUEUE,
  SERVER_TYPE_THREAD_POOL,
} server_type_t;

int
main(int argc, char *argv[])
{
  server_type_t server_type;
  short int port;
  int accept_fd;

  if (argc != 3) {
    printf("Proper usage of http server is:\n%s <port> <#>\n"
	   "port is the port to serve on, # is either\n"
	   "0: serve only a single request\n"
	   "1: use only a single thread for multiple requests\n"
	   "2: use fork to create a process for each request\n"
	   "3: Extra Credit: use fork and exec when the path is an executable to run the program dynamically.  This is how web servers handle dynamic (program generated) content.\n"
	   "4: create a thread for each request\n"
	   "5: use atomic instructions to implement a task queue\n"
	   "6: use a thread pool\n"
	   "7: to be defined\n"
	   "8: to be defined\n"
	   "9: to be defined\n",
	   argv[0]);
    return -1;
  }
	
  port = atoi(argv[1]);
  accept_fd = server_create(port);
  if (accept_fd < 0) return -1;
	
  server_type = atoi(argv[2]);
	
  switch(server_type) {
  case SERVER_TYPE_ONE:
    server_single_request(accept_fd);
    break;
  case SERVER_TYPE_SINGLET:
    server_multiple_requests(accept_fd);
    break;
  case SERVER_TYPE_PROCESS:
    server_processes(accept_fd);
    break;
  case SERVER_TYPE_FORK_EXEC:
    server_dynamic(accept_fd);
    break;
  case SERVER_TYPE_SPAWN_THREAD:
    server_thread_per(accept_fd);
    break;
  case SERVER_TYPE_TASK_QUEUE:
    server_task_queue(accept_fd);
    break;
  case SERVER_TYPE_THREAD_POOL:
    server_thread_pool(accept_fd);
    break;
  }
  close(accept_fd);
	
  return 0;
}

