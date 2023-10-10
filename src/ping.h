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
#include <sys/time.h>
#include <unistd.h>

#define NOT_ENOUGH_ARGUMENTS -1

typedef struct args_s {
    char *host;
    int verbose;
    int sweep_min;
    int sweep_step;
    int sweep_max;
} args_t;

struct context {
    char numeric_resolved_address[INET_ADDRSTRLEN + 1];

    struct sockaddr resolved_address;

    struct timespec start;
    struct timespec end;

    struct timeval timeout;

    double *rtts;

    char *requested_address;

    double time;

    double min;
    double avg;
    double max;

    int received_packets;
    int rtts_capacity;
    int sent_packets;
    int rtts_length;
    int data_size;
    int socketfd;
    int status;
    int sleep;
    int ttl;
};

#endif
