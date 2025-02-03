#ifndef PING_H
#define PING_H

#include <sys/types.h>

#include <arpa/inet.h>
#include <errno.h>
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
#include <time.h>
#include <unistd.h>

#define FLAG_HELP 0x1
#define FLAG_VERBOSE 0x2

struct config {
    char *host;
    uint16_t size;
    uint8_t ttl;
    char flags;
};

_Bool init_config_from_cli(int argc, char **argv, struct config *config);

struct sockinfo {
    struct sockaddr_in internet_address;
    char address[INET_ADDRSTRLEN];
    char *host;
    int socketfd;
};

_Bool dns_lookup(struct sockinfo *sockinfo);
_Bool create_and_configure_socket(struct sockinfo *sockinfo, struct config *config);
_Bool destroy_socket(struct sockinfo *sockinfo);

struct stats {
    uint32_t sent;
    uint32_t received;

    float min;
    float avg;
    float max;
};

void print_commence_message(struct sockinfo *sockinfo, struct config *config);

void print_statistics(struct sockinfo *sockinfo, struct stats *stats);

void print_icmp_err(int type, int code);

void print_ping(uint32_t n_bytes, char *host, uint32_t sequence, uint32_t ttl, float ms);

void int_handler(int _);

struct Node {
    struct Node *next;

    double x;
};

struct Node *list_prepend(struct Node *head, double x);
void list_free(struct Node *head);

double calculate_standard_deviation();

#endif
