//
//  client11b.c
//  datagram_socket_client11b
//
//  Created by Qian Wang on 9/8/18.
//  Copyright © 2018 Qian Wang. All rights reserved.
//
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
#include <sys/timeb.h>

#define SERVERPORT "10013"	// the port users will be connecting to
#define MAXBUFLEN 100

int msg_packet(unsigned char *msg, int seq_num, unsigned long time_stamp, char my_str[]);    //  packet all information to a char array

void digit_to_byte_array(long number, int bytes, char *results);  //  convert number to bytes array

void msg_unpack(unsigned char *msg, int msg_len, unsigned long *messages, char *buf);  //  unpack all information from reveived message

unsigned long bytes_array_to_digit(unsigned char *bytes_array, int length);  //  convert bytes array to a digital number

long get_time_msec();

int main(int argc, char *argv[])
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes, send_msg_len;
	struct sockaddr_storage server_addr;
	char A[MAXBUFLEN];
	char buf[1024] = "";
	unsigned char send_msg[1038];
	unsigned char receive_msg[1038];
    socklen_t serveraddr_len;
	unsigned long send_time, receive_time, diff_time;
	unsigned long messages[3];
    
    memset(&messages, -1, sizeof messages);

	printf("Please enter a string: ");
	fgets(A, MAXBUFLEN, stdin);
	A[strcspn(A, "\r\n")] = 0;

	if (argc != 2) {
		fprintf(stderr,"usage: Client hostname message\n");
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
			perror("Client: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "Client: failed to create socket\n");
		return 2;
	}

	send_time = get_time_msec();
	//printf("send_time: %lx\n", send_time);
	send_msg_len = msg_packet(&send_msg, 0, send_time, A);

	if ((numbytes = sendto(sockfd, send_msg, send_msg_len, 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("Client: sendto err");
		exit(1);
	}

	serveraddr_len = sizeof server_addr;
	
	//printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);
	
	if ((numbytes = recvfrom(sockfd, receive_msg, 1038, 0, (struct sockaddr*)&server_addr, &serveraddr_len)) == -1) {
		perror("Client: recvfrom err.");
    	exit(1);
	}

	// printf("receive_msg: ");
    // int j;
    // for (j = 0; j < numbytes; j++) {
    //     printf("%x ", receive_msg[j]);
    // }
    // printf("\n");

	msg_unpack(&receive_msg, strlen(A), &messages, &buf);

	//printf("send_time: %ld\n", messages[2]);
	receive_time = get_time_msec();
	//printf("receive_time: %ld\n", receive_time);
	diff_time = receive_time - messages[2];

	printf("Client: receive massage from server \"%s\"\n", buf);
	
	printf("Round trip time: %ld milliseconds\n", diff_time);
	

	freeaddrinfo(servinfo);
	
	close(sockfd);
	

	return 0;
}

// message packet function
// - packet all message to a bytes array
//      Tottal Message Length (bytes) | Sequence Number | Timestamp (ms) | String
//                  2 bytes           |     4 bytes     |  8 bytes(long) | Variable (Up to 1024 bytes)
// @param *msg, seq_num, time_stamp, my_str
int msg_packet(unsigned char *msg, int seq_num, unsigned long time_stamp, char my_str[]) {
    int msg_len = 14;
    unsigned char msg_len_bytes_array[2];
    unsigned char seq_bytes_array[4];
    unsigned char time_stamp_bytes_array[8];
    
    digit_to_byte_array(seq_num, 4, &seq_bytes_array);
    digit_to_byte_array(time_stamp, 8, &time_stamp_bytes_array);
    msg_len = msg_len + strlen(my_str);         // + 1 is for \0
    digit_to_byte_array(msg_len, 2, &msg_len_bytes_array);
    
    memcpy(msg, msg_len_bytes_array, sizeof(msg_len_bytes_array));
    memcpy(msg + 2, seq_bytes_array, sizeof(seq_bytes_array));
    memcpy(msg + 6, time_stamp_bytes_array, sizeof(time_stamp_bytes_array));
    memcpy(msg + 14, my_str, strlen(my_str));
    msg[msg_len] = 0;
    return (msg_len + 1);
}

//  digit to byte array function
//  - convert a digital number to a bytes array
void digit_to_byte_array(long number, int bytes, char *results) {
    int i;
    for (i =0; i < bytes; i++)
        results[i] = (number >> (i * 8)) & 0xff;
}

//  unpack the bytes array which is reeived from the server
//  -this function unpack reveiced message to Tottal Message Length (bytes); Sequence Number;  Timestamp (ms); Message
void msg_unpack(unsigned char *msg, int msg_len, unsigned long *messages, char *buf) {
    int total_len, seq_num, message;
    int i;
    long timestamp;
    unsigned char total_len_bytes_array[2];
    unsigned char seq_num_bytes_array[4];
    unsigned char timestamp_bytes_array[8];
    
    memset(total_len_bytes_array, 0 , sizeof total_len_bytes_array);
    memset(&seq_num_bytes_array, 0, sizeof seq_num_bytes_array);
    memset(&timestamp_bytes_array, 0, sizeof timestamp_bytes_array);
    
    for (i = 0; i < 2; i++)
        total_len_bytes_array[i] = msg[i];
    for (i = 0; i < 4; i++)
        seq_num_bytes_array[i] = msg[i + 2];
    for (i = 0; i < 8; i++)
        timestamp_bytes_array[i] = msg[i + 6];
    for (i = 0; i < msg_len; i++)
        buf[i] = msg[i + 14];
    
    total_len = bytes_array_to_digit(&total_len_bytes_array,2);
    seq_num = bytes_array_to_digit(&seq_num_bytes_array, 4);
    timestamp = bytes_array_to_digit(&timestamp_bytes_array, 8);
    
    messages[0] = total_len;
    messages[1] = seq_num;
    messages[2] = timestamp;
}


// bytes array to digit function
// - convert a bytes array to a long interger
unsigned long bytes_array_to_digit(unsigned char *bytes_array, int length) {
    unsigned long result = 0;
    int i;
    for (i = length - 1; i > -1; i--) {
        result = result + bytes_array[i];
        if (i != 0) {
            result = result << 8;
        }
    }
    return result;
}

// get current time in millisecond
long get_time_msec() {
	long result;
	struct timeb current_timeb;
	ftime(&current_timeb);
	result = current_timeb.time * 1000 + current_timeb.millitm;
	return result;
}