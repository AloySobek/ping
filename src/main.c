#include "ping.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FLAG_HELP 0x1
#define FLAG_VERBOSE 0x2

struct cli {
    char flags;
    char *host;
};

struct context context = {.timeout = {0},
                          .min = FLT_MAX,
                          .max = FLT_MIN,
                          .rtts_capacity = 128,
                          .max_replies = -1,
                          .data_size = 56,
                          .sleep = 1,
                          .ttl = 64};

int dns_lookup(char *addr, size_t len) {
    struct addrinfo hint;
    struct addrinfo *res;

    memset(&hint, 0, sizeof(struct addrinfo));

    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_RAW;
    hint.ai_protocol = IPPROTO_ICMP;

    if (!getaddrinfo(context.requested_address, NULL, &hint, &res) && res->ai_addr) {
        memcpy(&context.resolved_address, res->ai_addr, sizeof(struct sockaddr_in));

        struct sockaddr_in *resolved_address = (struct sockaddr_in *)&context.resolved_address;

        inet_ntop(AF_INET, &resolved_address->sin_addr, addr, len);

        addr[len] = '\0';

        freeaddrinfo(res);
    } else {
        return 1;
    }

    return 0;
}

unsigned short checksum(void *b, int len) {
    unsigned short *buf = b, result;
    unsigned int sum = 0;

    for (sum = 0; len > 1; len -= 2) {
        sum += *buf++;
    }

    if (len == 1) {
        sum += *(unsigned char *)buf;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    result = ~sum;

    return result;
}

double calculate_standard_deviation() {
    long double deviation_sum = 0.0f;

    for (struct Node *iter = context.rtts; iter; iter = iter->next) {
        deviation_sum += (iter->x - context.avg) * (iter->x - context.avg);
    }

    return sqrtf(deviation_sum / context.rtts_length);
}

int ping(char *addr, size_t len) {
    if ((context.socketfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
        printf("Error while creating socket: %d\n", context.socketfd);

        return context.socketfd;
    }

    if ((context.status =
             setsockopt(context.socketfd, IPPROTO_IP, IP_TTL, &context.ttl, sizeof(context.ttl)))) {
        printf("Error while setting IP time to live\n");

        return context.status;
    }

    if ((context.status = setsockopt(context.socketfd, SOL_SOCKET, SO_RCVTIMEO, &context.timeout,
                                     sizeof(context.timeout)))) {
        printf("Error while setting socket timeout\n");

        return context.status;
    }

    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &context.start);

    while (1) {
        struct icmp header = {0};

        char *data = (char *)malloc(sizeof(char) * context.data_size);

        memset(data, 0, context.data_size);

        header.icmp_type = ICMP_ECHO;
        header.icmp_hun.ih_idseq.icd_id = getpid();
        header.icmp_hun.ih_idseq.icd_seq = context.sent_packets++;

        // Tmp storage to put header and data in one place, because the data is dynamically
        // allocated
        char *buffer = (char *)malloc(sizeof(struct icmp) + context.data_size);

        memcpy(buffer, &header, sizeof(struct icmp));
        memcpy(buffer + sizeof(struct icmp), data, context.data_size);

        header.icmp_cksum = checksum(buffer, sizeof(struct icmp));

        memcpy(buffer, &header, sizeof(struct icmp));

        clock_gettime(CLOCK_MONOTONIC, &start);

        // sendto takes the buffer and fills the ip header itself, which is not how recvfrom works,
        // must be careful here
        if (sendto(context.socketfd, buffer, sizeof(struct icmp) + context.data_size, 0,
                   &context.resolved_address, sizeof(context.resolved_address)) <= 0) {
            printf("Error sending message");
        }

        struct sockaddr received_address;

        socklen_t len = sizeof(received_address);

        char reverse_resolved_address[NI_MAXHOST];
        char reverse_resolved_port[NI_MAXSERV];

        int bytes_received = 0;

        struct msghdr msg = {0};
        struct iovec iov[1];

        iov[0].iov_base = buffer;
        iov[0].iov_len = sizeof(struct icmp) + context.data_size;

        msg.msg_name = &received_address;
        msg.msg_namelen = len;
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;

        if ((bytes_received = recvmsg(context.socketfd, &msg, 0)) > 0) {
            clock_gettime(CLOCK_MONOTONIC, &end);

            // recvfrom returns ip header, so we must account for that
            int ip_header_len = (*buffer & 0x0F) * 4;

            memcpy(&header, buffer + ip_header_len, sizeof(struct icmp));

            if (header.icmp_type == ICMP_ECHOREPLY) {
                double rtt = (end.tv_sec - start.tv_sec) * 1000.0 +
                             (((double)(end.tv_nsec - start.tv_nsec)) / 1000000.0f);

                list_prepend(context.rtts, rtt);

                if (rtt < context.min) {
                    context.min = rtt;
                }

                context.avg = ((context.avg * context.received_packets) + rtt) /
                              (context.received_packets + 1);

                if (rtt > context.max) {
                    context.max = rtt;
                }

                ++context.received_packets;

                if (getnameinfo(&received_address, sizeof(struct sockaddr),
                                reverse_resolved_address, NI_MAXHOST, reverse_resolved_port,
                                NI_MAXSERV, 0)) {
                }

                printf("%d bytes from %s (%s): icmp_seq=%d ttl=%d, time=%.1f ms\n",
                       bytes_received - ip_header_len, reverse_resolved_address, addr,
                       context.sent_packets - 1, context.ttl, rtt);
            } else {
                printf("ICMP Type: %d; ICMP Code: %d\n", header.icmp_type, header.icmp_code);
            }
        }

        free(buffer);

        usleep(context.sleep * 1000000);
    }

    clock_gettime(CLOCK_MONOTONIC, &context.end);

    context.time = (context.end.tv_sec - context.start.tv_sec) * 1000.0 +
                   (((double)(context.end.tv_nsec - context.start.tv_nsec)) / 1000000.0f);

    return 0;
}

void int_handler(int _) {
    clock_gettime(CLOCK_MONOTONIC, &context.end);

    context.time = (context.end.tv_sec - context.start.tv_sec) * 1000.0 +
                   (((double)(context.end.tv_nsec - context.start.tv_nsec)) / 1000000.0f);

    printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n", context.min, context.avg, context.max,
           calculate_standard_deviation());

    exit(0);
}

int app(struct cli *cli) {
    char addr[INET_ADDRSTRLEN + 1];

    if (dns_lookup(addr, INET_ADDRSTRLEN + 1)) {
        printf("ping: %s: Name or service not known\n", context.requested_address);

        return -1;
    }

    char buffer[255];

    sprintf(buffer, ", id 0x%04x = %d", getpid(), getpid());

    printf("PING %s (%s) %d(%d) bytes of data%s\n", context.requested_address, addr,
           context.data_size, (int)(context.data_size + sizeof(struct icmp)),
           (cli->flags & FLAG_VERBOSE ? buffer : ""));

    ping(addr, INET_ADDRSTRLEN + 1);

    printf("\n--- %s ping statistics ---\n", context.requested_address);
    printf("%d packets transmitted, %d received, %.1f%% packet loss, time %.0fms\n",
           context.sent_packets, context.received_packets,
           (100.0f / ((float)context.sent_packets)) *
               (context.sent_packets - context.received_packets),
           context.time);

    if (!context.received_packets) {
        return -1;
    }

    printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n", context.min, context.avg, context.max,
           calculate_standard_deviation());

    return 0;
}

int8_t parse_cli(int argc, char **argv, struct cli *cli) {
    if (cli == NULL) {
        return -1;
    }

    int option;

    while ((option = getopt(argc, argv, "v")) != -1) {
        switch (option) {
        case 'v': {
            cli->flags |= FLAG_VERBOSE;
            break;
        }
        case 'h': {
            cli->flags = FLAG_HELP;
            break;
        }
        default: {
            break;
        }
        }
    }

    if (optind < argc) {
        cli->host = argv[optind];
    }

    return 0;
};

int main(int argc, char **argv) {
    struct cli cli = {0, NULL};
    int8_t err;

    if ((err = parse_cli(argc, argv, &cli) != 0)) {
        return err;
    }

    if (cli.flags & FLAG_HELP) {
        printf(
            "Usage: ft_ping [OPTION...] HOST ...\nSend ICMP ECHO_REQUEST packets to network hosts");

        return 0;
    }

    if (cli.host == NULL) {
        printf("ft_ping: missing host operand\nTry 'ft_ping' -? for more information\n");

        return -1;
    }

    context.requested_address = cli.host;

    signal(SIGINT, int_handler);

    app(&cli);

    return 0;
}
