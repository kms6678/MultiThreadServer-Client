#include "stems.h"
#include "request.h"
#define Q_SIZE 10000
// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

typedef struct {
  int fd;
  long size, arrival, dispatch;
} request;

request queue[10000];
int front = 0;
int rear = 0;

enum {FIFO, HPSC, HPDC};

request **buffer;
sem_t items;
sem_t BufferIsEmpty;
sem_t count_sema;

pthread_mutex_t  mutex = PTHREAD_MUTEX_INITIALIZER;

request _req;
request dequeue()
{
	request ret = queue[front];
	front = (front + 1) % Q_SIZE;
	return ret;
}
void enqueue(request value)
{
	queue[rear] = value;
	rear = (rear + 1) % Q_SIZE;
}

void getargs(int *port, int *threads, int *buffers, int *alg, int argc, char *argv[])
{
  if (argc != 3) { /* You will change 2 to 5 */
    fprintf(stderr, "Usage: %s <port> <NumOfThreads>\n", argv[0]);
    exit(1);
  }
  *port = atoi(argv[1]);
  *threads = atoi(argv[2]);
  *buffers = 1;
  *alg = FIFO;
}

void consumer(request *req) {
  sem_post(&BufferIsEmpty);
  
  struct timeval dispatch;
  gettimeofday(&dispatch, NULL);
  req->dispatch = ((dispatch.tv_sec) * 1000 + dispatch.tv_usec/1000.0) + 0.5;
  //printf("                                  before RQH : \n");
  requestHandle(req->fd, req->arrival, req->dispatch);
  //printf("                                  after RQH : \n");
  Close(req->fd);
  //printf("Close connection. File descriptor fd is %d\n", req->fd);
}

void* worker(void* arg)
{
	request req;
	while (1){
		sem_wait(&items); // block if no item exists.
		//printf("start!!!\n");
		pthread_mutex_lock(&mutex);
	    req =_req;
		pthread_mutex_unlock(&mutex);

		consumer(&req);	

		sem_wait(&count_sema);
		request_count++;
		printf("request count : %d\n",request_count);
		sem_post(&count_sema);
	}
}

int main(int argc, char *argv[])
{
  //server_start time
  gettimeofday(&server_start, NULL);
  int i, N;
  int listenfd, connfd, port, buffers, alg, clientlen;
  struct sockaddr_in clientaddr;
  struct timeval arrival;
  //request req;
  
  pthread_t* threads;
  getargs(&port, &N, &buffers, &alg, argc, argv);

  threads = (pthread_t*)malloc(sizeof(pthread_t)*N);
  sem_init(&items, 0, 0);
//sem_init(&BufferIsEmpty, 0, Q_SIZE);
  sem_init(&BufferIsEmpty, 0, 1);
  sem_init(&count_sema,0,1);
  
  for (i = 0; i < N; i++){
	  if (pthread_create(&(threads[i]), NULL, &(worker), NULL) < 0)
	  {
		  fprintf(stderr, "Thread initiation error\n");
		  return 0;
	  }
  }
  
  listenfd = Open_listenfd(port);
  //clientlen = sizeof(clientaddr);
  while (1) {	  
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);
	
	sem_wait(&BufferIsEmpty); // block if no free thread exists
	
	//printf("New connection. File descriptor fd is %d\n\n", connfd);
	gettimeofday(&arrival, NULL);
	_req.fd = connfd;
	_req.arrival = ((arrival.tv_sec) * 1000 + arrival.tv_usec / 1000.0) + 0.5;

	//sem_wait(&BufferIsEmpty); // block if no free thread exists

	pthread_mutex_lock(&mutex);	
	_req.fd = connfd;
	pthread_mutex_unlock(&mutex);

	sem_post(&items); // add new item = wake one thread
  }

}
