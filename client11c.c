//
//  client11c.c
//  datagram_socket_client11c
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
#include <sys/types.h>
#include <time.h>
#include <limits.h>
#include <stdbool.h>


#define SERVERPORT "10010"

#define MAXBUFLEN 100


void child_process(int sockfd, struct addrinfo *servinfo);               //  child process prototype.

void parent_process(int sockfd, struct sockaddr_storage *server_addr, socklen_t *serveraddr_len);   //  parent process prototype.

int msg_packet(unsigned char *msg, int seq_num, long time_stamp, int my_num);    //  packet all information to a char array

void digit_to_byte_array(long number, int bytes, char *results);  //  convert number to bytes array

void msg_unpack(unsigned char *msg, int msg_len, long *messages);  //  unpack all information from reveived message

long bytes_array_to_digit(unsigned char *bytes_array, int length);  //  convert bytes array to a digital number

int main(int argc, char *argv[]) {
    int pid;
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    struct sockaddr_storage server_addr;
    struct timeval timeout;
    socklen_t serveraddr_len;
    
    timeout.tv_sec = 5;  //  receive function timeout
    timeout.tv_usec = 0;
    
    if (argc != 2) {
        fprintf(stderr,"Client: Missing server hostname.\n");
        exit(1);
    }
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "Client getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("Client: create socket unsuccessfully");
            continue;
        }
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr, "Client: failed to bind socket\n");
        return 2;
    }
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) != 0) {
        perror("Client: set socket opt unsuccessfully");
        exit(1);
    }
    
    pid = fork();
    
    if (pid == 0) {
        child_process(sockfd, p);
    }
    else {
        serveraddr_len = sizeof server_addr;
        parent_process(sockfd, &server_addr, &serveraddr_len);
    }
    
    freeaddrinfo(servinfo);
    close(sockfd);
    return 0;
}

//  Child process
//  - Send message to server
void child_process(int sockfd, struct addrinfo *servinfo) {
    int numbytes, i;
    int seq_num, my_num, msg_length;
    long time_stamp;
    unsigned char message[1038];
    
    printf("Client: start to send meg.\n");
    for (i = 0; i < 10000; i++) {
        my_num = i + 1;
        seq_num = i;
        time_stamp = time(NULL);
        msg_length = msg_packet(&message, seq_num, time_stamp, my_num);
        if ((numbytes = sendto(sockfd, message, msg_length, 0, servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
            perror("Client: send message error");
            exit(1);
        }
    }
    
    printf("Client: finish sending mes.\n");
}


//  Parent process
//  - Receiving responses from server and reporting missing echoes;
//  - Calculating the smallest, largest, and average round trip times;
void parent_process(int sockfd, struct sockaddr_storage *server_addr, socklen_t *serveraddr_len) {
    int iter = 0;
    int numbytes, i;
    long recv_times[10000];
    long send_times[10000];
    long messages[4];            // 0 - Tottal Message Length (bytes); 1 - Sequence Number; 2 - Timestamp (ms); 3 - String
    unsigned char recv_meg[MAXBUFLEN];
    int samples[10001];
    int missing_echoes = 0;
    double small = __DBL_MAX__;
    double large = __DBL_MIN__;
    double average = 0;
    double differ = 0;
    
    memset(&samples, 0, sizeof(samples));
    memset(&recv_times, -1, sizeof(recv_times));
    memset(&send_times, -1, sizeof(send_times));
    memset(&messages, -1, sizeof messages);
    
    while (true) {
        if ((numbytes = recvfrom(sockfd, recv_meg, MAXBUFLEN - 1, 0, (struct sockaddr *)server_addr, serveraddr_len)) == -1) {
            perror("Client: receiver timeout.");
            break;
        }
        msg_unpack(&recv_meg, numbytes, &messages);
        if (messages[1] >=0 && messages[1] <= 10000){
            samples[(int)messages[3]] = 1;
            send_times[(int)messages[1]] = messages[2];
            recv_times[(int)messages[1]] = time(NULL);
        }
    }
    
    for (i = 1; i< 10001; i++) {
        if (samples[i] == 0) {
            //printf("Missing echo: %d\n", i);
            missing_echoes++;
        }
        else {
            differ = difftime(recv_times[i - 1], send_times[i - 1]);
            if (differ > large)
                large = differ;
            if (differ < small)
                small = differ;
            average = average + differ;
            iter++;
        }
    }
    average = average / iter;
    printf("Client: missing echoes is %d\n", missing_echoes);
    printf("Client: the smallest round trip time is %f.\n", small);
    printf("Client: the largest round trip time is %f.\n", large);
    printf("Client: the average round trip time is %f.\n", average);
    
}

// message packet function
// - packet all message to a bytes array
//      Tottal Message Length (bytes) | Sequence Number | Timestamp (ms) | String
//                  2 bytes           |     4 bytes     |  8 bytes(long) | Variable (Up to 1024 bytes)
//
int msg_packet(unsigned char *msg, int seq_num, long time_stamp, int my_num) {
    int msg_len = 14;
    unsigned char msg_len_bytes_array[2];
    unsigned char seq_bytes_array[4];
    unsigned char time_stamp_bytes_array[8];
    char my_num_str[1024];
    
    digit_to_byte_array(seq_num, 4, &seq_bytes_array);
    digit_to_byte_array(time_stamp, 8, &time_stamp_bytes_array);
    sprintf(my_num_str, "%d", my_num);
    msg_len = msg_len + strlen(my_num_str);         // + 1 is for \0
    digit_to_byte_array(msg_len, 2, &msg_len_bytes_array);
    
    memcpy(msg, msg_len_bytes_array, sizeof(msg_len_bytes_array));
    memcpy(msg + 2, seq_bytes_array, sizeof(seq_bytes_array));
    memcpy(msg + 6, time_stamp_bytes_array, sizeof(time_stamp_bytes_array));
    memcpy(msg + 14, my_num_str, strlen(my_num_str));
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
void msg_unpack(unsigned char *msg, int msg_len, long *messages) {
    int total_len, seq_num, message;
    int i;
    long timestamp;
    unsigned char total_len_bytes_array[2];
    unsigned char seq_num_bytes_array[4];
    unsigned char timestamp_bytes_array[8];
    char message_str[1024] = "";
    
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
        message_str[i] = msg[i + 14];
    
    total_len = bytes_array_to_digit(&total_len_bytes_array,2);
    seq_num = bytes_array_to_digit(&seq_num_bytes_array, 4);
    timestamp = bytes_array_to_digit(&timestamp_bytes_array, 8);
    message = atoi(message_str);
    
    messages[0] = total_len;
    messages[1] = seq_num;
    messages[2] = timestamp;
    messages[3] = message;
}


// bytes array to digit function
// - convert a bytes array to a long interger
long bytes_array_to_digit(unsigned char *bytes_array, int length) {
    int result = 0;
    int i;
    for (i = length - 1; i > -1; i--) {
        result = result + bytes_array[i];
        if (i != 0) {
            result = result << 8;
        }
    }
    return result;
}
