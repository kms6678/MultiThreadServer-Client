/*
* client.c: A very, very primitive HTTP client.
*
* To run, try:
*      client pintos.kumoh.ac.kr 80 /~choety/
*
* Sends one HTTP request to the specified HTTP server.
* Prints out the HTTP response.
*
* For testing your server, you will want to modify this client.
*
* When we test your server, we will be using modifications to this client.
*
*/

#include "stems.h"
#include "unistd.h"
#include "sys/types.h"
#include "semaphore.h"

pthread_barrier_t barrier;
static int _pidx = 0;
sem_t* arr_sema;
sem_t idx_sema;
sem_t count_sema;
char buffer__[100000][100];
long first_request_arrival = 0;
long last_request_complete;

int s;
/*
* Send an HTTP request for the specified file
*/
void clientSend(int fd, char *filename)
{
	//printf("clientsend() call\n");
	char buf[MAXLINE];
	char hostname[MAXLINE];

	Gethostname(hostname, MAXLINE);

	/* Form and send the HTTP request */
	sprintf(buf, "GET %s HTTP/1.1\n", filename);
	sprintf(buf, "%shost: %s\n\r\n", buf, hostname);
	Rio_writen(fd, buf, strlen(buf));
}

/*
* Read the HTTP response and print it out
*/
void clientPrint(int fd)
{
	//printf("clientPrint() call\n");
	rio_t rio;
	char buf[MAXBUF];
	int n;

	Rio_readinitb(&rio, fd);

	/* Read and display the HTTP Header */
	n = Rio_readlineb(&rio, buf, MAXBUF);

	while (strcmp(buf, "\r\n") && (n > 0)) {
		printf("Header: %s", buf);
		n = Rio_readlineb(&rio, buf, MAXBUF);

		/* If you want to look for certain HTTP tags... */
		//if (sscanf(buf, "Content-Length: %d ", &length) == 1) {
		//	printf("Length = %d\n", length);
		//}
		//printf("@@@@      %s",buf);
		//if (first_request_arrival == 0){
		//	sscanf(buf, "Stat-req-arrival: %ld", &first_request_arrival);
		//	printf("first_request_arrival = %ld\n", first_request_arrival);
		//}		
		//if (sscanf(buf, "Stat-req-complete: %ld", &last_request_complete) == 1) {
		//	printf("last_request_complete = %ld\n", last_request_complete);
		//}
	}

	/* Read and display the HTTP Body */
	n = Rio_readlineb(&rio, buf, MAXBUF);
	while (n > 0) {
		printf("%s", buf);		
		n = Rio_readlineb(&rio, buf, MAXBUF);
		if (first_request_arrival == 0){
			sscanf(buf, "Stat-req-arrival: %ld", &first_request_arrival);
			//printf("first_request_arrival = %ld\n", first_request_arrival);
		}		
		if (sscanf(buf, "Stat-req-complete: %ld", &last_request_complete) == 1) {
			printf("last_request_complete = %ld\n", last_request_complete);
		}
	}	
}

int getrand(void)
{
  srand (time (NULL));
  int r = rand()%5;
  return r+1;
}

typedef struct _argData
{
	char *host, *filename, *filename2;
	int port;
	int N;
	int M;
	char* schedalg;
} argData;

void* worker(void* arg);

int main(int argc, char *argv[])
{	
	struct timeval start, end;
	//long start_t, end_t;
	char *host, *filename, *filename2 = NULL;
	int port;
	int N;
	int M;
	int i;
	char* schedalg;
	long total_time;
	//long total_time2;

	if (argc < 7) {
		fprintf(stderr, "Usage: %s <host> <port> <N> <M> <schedalg> <filename>\n", argv[0]);
		exit(1);
	}
	
	host = argv[1];
	port = atoi(argv[2]);
	N = atoi(argv[3]);
	M = atoi(argv[4]);
	schedalg = argv[5];
	filename = argv[6];

	if(argc == 8)
	{
		filename2 = argv[7];
	}

	pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t)*N);
	pthread_barrier_init(&barrier, NULL, N);
	

	argData data;
	data.host = host;
	data.port = port;
	data.filename = filename;
	data.filename2 = filename2;
	data.N = N;
	data.M = M;
	data.schedalg = schedalg;

	if (schedalg[0] == 'F')
	{
		arr_sema = (sem_t*)malloc(sizeof(sem_t)*N);
		for (i = 0; i<N; i++)
			sem_init(&arr_sema[i], 0, 0);
		sem_init(&idx_sema,0,1);
	}
	
	sem_init(&count_sema,0,1);
	
	gettimeofday(&start, NULL);
	//start_t = (((start.tv_sec) * 1000 + (start.tv_usec) / 1000.0) + 0.5);

	for (i = 0; i<N; i++)
	{
		if (pthread_create(&(threads[i]), NULL, &(worker), (void*)&data) < 0)
		{
			fprintf(stderr, "Thread initiation error\n");
			return 0;
		}
	}
	if (schedalg[0] == 'F')
	{
		sem_post(&arr_sema[0]);
	}
	
	int status;
	int j;
	for (j = 0; j<N; j++)
	{
		pthread_join(threads[j], (void*)&status);
	}
	
	gettimeofday(&end, NULL);
	//end_t = (((end.tv_sec) * 1000 + (end.tv_usec) / 1000.0) + 0.5);
	//total_time2 = end_t - start_t;
	printf("request_count : %d\n", request_count);
	printf("last_request_complete : %ld\n", last_request_complete);
	printf("first_request_arrival : %ld\n", first_request_arrival);
	total_time = last_request_complete - first_request_arrival;
	printf("total_time  : %ld\n", total_time);
	//printf("total_time2 : %ld\n", total_time2);
	printf("throughput : %f\n", (float)request_count / (float)total_time);
	
	exit(0);
}

void* worker(void* arg)
{
	//printf("%d\n", (int)pthread_self());
	argData* argv = (argData*)arg;

	int clientfd = 0;
	char* host = argv->host;
	int port = argv->port;
	char* filename = argv->filename;
	char* filename2 = argv->filename2;
	int N = argv->N;
	int M = argv->M;
	char* schedalg = argv->schedalg;
	int i;
	static int count = 0;
	int flag=1;
	int _isalternately = 0;
	
	if(filename2!=NULL)
		_isalternately = 1;
	
	switch (schedalg[0])
	{
		case 'C':
		{			
					for (i = 0; i<M; i++)
					{
						clientfd = Open_clientfd(host, port);
						//printf("\"%d\"st call\n", i);
						/* Open a single connection to the specified host and port */		
						
						if(_isalternately == 0)
							clientSend(clientfd, filename);
						else
						{
							if(flag == 1)	
							{		
								clientSend(clientfd, filename);
								flag++;
							}
							else if(flag == 2)
							{
								clientSend(clientfd, filename2);
								flag--;
							}
						}
						
						clientPrint(clientfd);
						
						sem_wait(&count_sema);
						request_count++;
						sem_post(&count_sema);
						
						Close(clientfd);
						pthread_barrier_wait(&barrier);
					}
					//Close(clientfd);
					break;
		}
		case 'F':
		{			sem_wait(&idx_sema);
					int pidx = _pidx++;
					sem_post(&idx_sema);
					for (i = 0; i<M; i++)
					{					
						sem_wait(&arr_sema[pidx]); // wait (N)					

						clientfd = Open_clientfd(host, port);
						sprintf(buffer__[count++], "tid = %d\n", (int)pthread_self());				
							
						if(_isalternately == 0)
							clientSend(clientfd, filename);
						else
						{
							if(flag == 1)	
							{		
								clientSend(clientfd, filename);
								flag++;
							}
							else if(flag == 2)
							{
								clientSend(clientfd, filename2);
								flag--;
							}
						}

						sem_post(&arr_sema[(pidx+1)%N]); // wake (N+1)
											
						clientPrint(clientfd);
											
						sem_wait(&count_sema);
						request_count++;
						sem_post(&count_sema);
						
						Close(clientfd);
						pthread_barrier_wait(&barrier); // wait the others
					}
					break;
		}
		case 'R':
		{
					for (i = 0; i<M; i++)
					{
						//printf("\"%d\"st call\n", i);
						clientfd = Open_clientfd(host, port);
						
						if(_isalternately == 0)
							clientSend(clientfd, filename);
						else
						{
							if(flag == 1)	
							{		
								clientSend(clientfd, filename);
								flag++;
							}
							else if(flag == 2)
							{
								clientSend(clientfd, filename2);
								flag--;
							}
						}
						
						clientPrint(clientfd);
											
						sem_wait(&count_sema);
						request_count++;
						sem_post(&count_sema);
						
						sleep(getrand());
					}
					break;
		}
	}
	//printf("%d , %s , %d, %s\n",clientfd,filename,M,schedalg);
	return (void*)0;
}
