#ifndef PING_H
#define PING_H

#include <sys/types.h>

#include <arpa/inet.h>
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
    int ttl;
    int sleep;
    int status;
    int socketfd;
    int sent_messages_count;
    int received_messages_count;

    struct timeval timeout;

    char *requested_address;

    struct sockaddr resolved_address;

    char reverse_resolved_address[NI_MAXHOST];
    char reverse_resolved_port[NI_MAXSERV];
};

struct header {
    struct icmp header;

    char message[64 + sizeof(struct icmp)];
};

#endif
