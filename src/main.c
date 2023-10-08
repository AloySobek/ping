#include "ping.h"

volatile static int running = 1;

// Initialize ping state
struct context *create_context(char *requested_address) {
    struct context *context = (struct context *)malloc(sizeof(struct context));

    memset(context, 0, sizeof(struct context));

    context->ttl = 64;
    context->sleep = 1;

    context->timeout.tv_sec = 5;

    context->requested_address = requested_address;

    return context;
}

// Free ping state
void free_context(struct context *context) { free(context); }

// Performs dns lookup, uses raw socket and icmp protocol as search hints
int dns_lookup(struct context *context) {
    struct addrinfo hint;
    struct addrinfo *res;

    memset(&hint, 0, sizeof(struct addrinfo));

    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_RAW;
    hint.ai_protocol = IPPROTO_ICMP;

    if (!getaddrinfo(context->requested_address, NULL, &hint, &res) && res->ai_addr) {
        memcpy(&context->resolved_address, res->ai_addr, sizeof(struct sockaddr_in));

        freeaddrinfo(res);
    } else {
        return 1;
    }

    return 0;
}

// Performs reverse dns lookup
int reverse_dns_lookup(struct context *context) {
    if (getnameinfo(&context->resolved_address, sizeof(struct sockaddr),
                    context->reverse_resolved_address, NI_MAXHOST, context->reverse_resolved_port,
                    NI_MAXSERV, 0)) {
        return 1;
    }

    return 0;
}

// Calculating the Check Sum
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int ping(struct context *context) {
    if ((context->socketfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
        printf("Error while creating socket: %d\n", context->socketfd);

        return context->socketfd;
    }

    if ((context->status = setsockopt(context->socketfd, IPPROTO_IP, IP_TTL, &context->ttl,
                                      sizeof(context->ttl)))) {
        printf("Error while setting IP time to live\n");

        return context->status;
    }

    if ((context->status = setsockopt(context->socketfd, SOL_SOCKET, SO_RCVTIMEO, &context->timeout,
                                      sizeof(context->timeout)))) {
        printf("Error while setting socket timeout\n");

        return context->status;
    }

    struct timespec start, end;

    while (running) {
        usleep(context->sleep * 1000000);

        struct icmp header;

        memset(&header, 0, sizeof(header));

        header.icmp_type = ICMP_ECHO;
        header.icmp_hun.ih_idseq.icd_id = getpid();
        header.icmp_hun.ih_idseq.icd_seq = context->sent_messages_count++;

        header.icmp_cksum = checksum(&header, sizeof(header));

        clock_gettime(CLOCK_MONOTONIC, &start);

        if (sendto(context->socketfd, &header, sizeof(header), 0, &context->resolved_address,
                   sizeof(context->resolved_address)) <= 0) {
            printf("Error sending message");
        }

        struct sockaddr received_address;

        socklen_t len = sizeof(received_address);

        if (recvfrom(context->socketfd, &header, sizeof(header), 0, &received_address, &len) <= 0) {
            printf("Error receiving\n");
        } else {
            clock_gettime(CLOCK_MONOTONIC, &end);

            double timeElapsed = ((double)(end.tv_nsec - start.tv_nsec)) / 1000000.0;

            long double rtt_msec = (end.tv_sec - start.tv_sec) * 1000.0 + timeElapsed;

            printf("%Lf\n", rtt_msec);
        }
    }

    return 0;
}

void int_handler(int _) { running = 0; }

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Not enough arguments\n");

        return 0;
    }

    signal(SIGINT, int_handler);

    struct context *context = create_context(argv[1]);

    if (dns_lookup(context)) {
        printf("DNS lookup failed\n");

        return 0;
    }

    char str[INET_ADDRSTRLEN + 1];

    struct sockaddr_in *resolved_address = (struct sockaddr_in *)&context->resolved_address;

    inet_ntop(AF_INET, &resolved_address->sin_addr, str, sizeof(str));

    str[INET_ADDRSTRLEN] = '\0';

    printf("Resolved address: %s\n", str);

    if (reverse_dns_lookup(context)) {
        printf("Reverse DNS lookup failed\n");
    }

    printf("Reverse resolved address: %s:%s\n", context->reverse_resolved_address,
           context->reverse_resolved_port);

    ping(context);

    free_context(context);

    return 0;
}
