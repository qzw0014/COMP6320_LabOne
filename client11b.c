/*
** talker.c -- a datagram "client" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

#define SERVERPORT "10010"	// the port users will be connecting to
#define MAXBUFLEN 100

int main(int argc, char *argv[])
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage server_addr;
    char buf[MAXBUFLEN];
	char A[MAXBUFLEN];
	char msg[MAXBUFLEN];
    socklen_t serveraddr_len;
	time_t send_time, receive_time;
	double seconds;

	printf("Please enter a string:");
	fgets(A, MAXBUFLEN, stdin);
	A[strcspn(A, "\r\n")] = 0;

	if (argc != 2) {
		fprintf(stderr,"usage: talker hostname message\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}

	if ((numbytes = sendto(sockfd, A, strlen(A), 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("client_test: sendto err");
		exit(1);
	}
	
	send_time = time(NULL);

	serveraddr_len = sizeof server_addr;
	
	//printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);
	
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1, 0, (struct sockaddr*)&server_addr, &serveraddr_len)) == -1) {
		perror("client_test: recvfrom err.");
    	exit(1);
	}
	receive_time = time(NULL);
	printf("client: receive massage from server \"%s\"\n", buf);
	seconds = difftime(receive_time, send_time);
	printf("Round trip time: %.f seconds\n", seconds);
	

	freeaddrinfo(servinfo);
	
	close(sockfd);
	

	return 0;
}
