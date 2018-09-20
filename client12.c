/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>

#include <arpa/inet.h>

#define PORT "3498" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

//By Yunfan
void bundle(unsigned int a, unsigned int b, unsigned char c, char* message); 
unsigned int getResult(char* buf, int index);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	//By Yunfan
	unsigned int a,b,result;
	unsigned char c;
	char message[9];
	int len;
	int i;
	

	if (argc != 5) {
		fprintf(stderr,"usage: client hostname\n");
		printf("The number of command arguments should be 5, yours are %d\n", argc);
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	//By Yunfan
	a = (unsigned)atoi(argv[2]);
	b = (unsigned)atoi(argv[3]);
	c = argv[4][0];
	printf("a = %u\n", a);
	printf("b = %u\n", b);
	printf("c = %c\n", c);
	if(!(c == '+' || c== '-' || c== '*' || c == '/') ){
		printf("c should be +, -, * or /.");
		exit(1);
	}

	//By Yunfan
	//bundle the a b c into the message
	bundle(a, b, c, message);  
	len = sizeof(message) / sizeof(message[0]);
	//printf("len = %d", len);
	//int i = 0;
	//for(i = 0; i < len; i++)
		//printf("message: %x\n", message[i]);

	//By Yunfan, send the message to the server
	if (send(sockfd, message, len, 0) == -1) {
		perror("client12: send");
		exit(1);
	}
	printf("client12: The message has sent to the server.\n");

	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}

	buf[numbytes] = '\0';

	//printf("client: received '%s'\n",buf);
	//for(i = 0; i < numbytes - 1; i++)
		//printf("client12: buf[%d] = %x\n", i, buf[i]);
	//rintf("client12: buf[%d] = %c\n", 13, buf[13]);

	result = getResult(buf, 9); // The result part begin at index = 9 in buf.
	printf("The result received from server12 is %u\n", result);

	close(sockfd);

	return 0;
}

//Yunfan
/************   bundle the a,b,c into the message   ***************
 * unsigned int a: a operand read from command line argument
 * unsigned int b: a operand read from command line argument
 * unsigned char c: a operator read from command line argument
 * char * message: The message which will be sent to the server
 * **/
void bundle(unsigned int a, unsigned int b, unsigned char c, char* message) {
	int k = 24;
    int i = 0;
	
	message[i] = c;
	//printf("message[%d]: %x\n", i, message[i]);
	i++;

    while(k >= 0) {
        message[i] = (int) ((a >> k) & 0xFF);
        //printf("message[%d]: %x\n", i, message[i]);
        i++;
        k -= 8;
	}
	
	k = 24;
	while(k >= 0) {
        message[i] = (int) ((b >> k) & 0xFF);
        //printf("message[%d]: %x\n", i, message[i]);
        i++;
        k -= 8;
	}
	
	
}

//Yunfan
/************   bundle the a,b,c into the message   ***************
 * char* buf: The string buf is received from server
 * int index: The begin index for result
 * return: the unsigned int result
 * **/
unsigned int getResult(char* buf, int index) {
	unsigned int result = 0;
	int i = index;

	while(i < (index + 4)){
		result *= 16;
		result += (unsigned int)buf[i];
		i++;
	}
	//printf("result = %u\n", result);
	return result;
}
