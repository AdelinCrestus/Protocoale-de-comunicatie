#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>

#define MAX_LEN_CONTENT 1500
#define MAX_UDP_MSG 1560

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

typedef struct udp_data
{
    char topic[50];
    uint8_t tip_date;
    char continut[MAX_LEN_CONTENT];
}T_udp_data;

typedef struct queue_msg
{
    char *msg;
    struct queue_msg *next;
    int len;
}TQmsg;

typedef struct client
{
    char id[11];
    int connfd;
    struct sockaddr_in *addr;
    int *sf;
    int enable;
    TQmsg *q_data; // coada de trimis pt at cand nu e activ
    TQmsg *last;
    struct client **next; // pt lista din topic
}Tclient;

typedef struct topic
{
    char topic[50];
    Tclient **head;
    Tclient **last;
}Ttopic;



int exist_topic(Ttopic **topics,char *topic, int nr_topics)
{
    for(int i = 0; i < nr_topics ; i++)
    {
        if( strcmp(topics[i]->topic, topic) == 0)
        {
            return i;
        }
    }
    return -1;

}