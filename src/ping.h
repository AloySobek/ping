#ifndef PING_H
#define PING_H

#include <sys/types.h>

#include <arpa/inet.h>
#include <float.h>
#include <math.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define NOT_ENOUGH_ARGUMENTS -1

struct Node {
    struct Node *next;

    double x;
};

typedef struct args_s {
    char *host;
    int verbose;
    int sweep_min;
    int sweep_step;
    int sweep_max;
} args_t;

typedef struct context {
    struct sockaddr resolved_address;

    struct timespec start;
    struct timespec end;

    struct timeval timeout;

    struct Node *rtts;

    char *requested_address;

    double time;

    double min;
    double avg;
    double max;

    int received_packets;
    int rtts_capacity;
    int sent_packets;
    int max_replies;
    int rtts_length;
    int data_size;
    int socketfd;
    int status;
    int sleep;
    int ttl;
} context_t;

extern struct context context;

int handle_arguments(int argc, char **argv);

struct Node *list_prepend(struct Node *head, double x);
void list_free(struct Node *head);

#endif
