#include "stems.h"
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>


//
// This program is intended to help you test your web server.
// You can use it to test that you are correctly having multiple threads
// handling http requests.
// 

double spinfor = 5.0;

int getrand(int r1, int r2)
{
  int r;
  
  while(1)
  {
	srand (time (NULL));
	r = rand()%r2;
	if(r1<=r+1)
	  break;
  }
  
  return r+1;
}

void getargs()
{
	char *buf, *p;
	int idx = 0;
	double range[2];
	/* Extract the four arguments */
	buf = getenv("QUERY_STRING");
	if (buf != NULL) {
		p = strtok(buf, "&");
		do
		{
			if (strcmp(p, "0") == 0)
				break;
			range[idx++] = (double)atoi(p);
		} while ( (p = strtok(NULL, "&")) != NULL );
		spinfor = getrand(range[0],range[1]);
		return;
	}
}

double Time_GetSeconds() {
	struct timeval t;
	int rc = gettimeofday(&t, NULL);
	assert(rc == 0);
	return (double)((double)t.tv_sec + (double)t.tv_usec / 1e6);
}


int main(int argc, char *argv[])
{
	char content[MAXBUF];
	getargs(); // make spinfor
	
	
	//while ((Time_GetSeconds() - t1) < spinfor) 
	sleep(spinfor);
	

	/* Make the response body */
	sprintf(content, "<html>\r\n");
	sprintf(content, "%s<p>Welcome to the CGI program</p>\r\n", content);
	sprintf(content, "%s<p>My only purpose is to waste time on the server!</p>\r\n", content);
	sprintf(content, "%s<p>I spun for %.2f seconds</p>\r\n", content, spinfor);
	//sprintf(content, "%s<p>%s</p>\r\n", content, getenv("QUERY_STRING"));
	sprintf(content, "%s</html>\r\n", content);

	/* Generate the HTTP response */
	printf("Content-length: %lu\r\n", strlen(content));
	printf("Content-type: text/html\r\n");
	printf("%s", content);
	fflush(stdout);

	exit(0);
}
