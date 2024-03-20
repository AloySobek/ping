#include "ping.h"

struct context context;

int init_context() {
    context.timeout.tv_sec = 1;

    if (!(context.rtts = (double *)malloc(sizeof(double) * 128))) {
        printf("Memory allocation error\n");

        return -1;
    }

    context.min = FLT_MAX;
    context.avg = 0.0f;
    context.max = FLT_MIN;

    context.rtts_capacity = 128;
    context.max_replies = -1;
    context.data_size = 56;
    context.sleep = 1;
    context.ttl = 64;

    return 0;
}

void free_context() { free(context.rtts); }

void insert_rtt(double rtt) {
    if (context.rtts_length + 1 < context.rtts_capacity) {
        context.rtts[context.rtts_length++] = rtt;
    } else {
        context.rtts_capacity *= 2;

        double *rtts = (double *)malloc(sizeof(double) * context.rtts_capacity);

        for (int i = 0; i < context.rtts_length; ++i) {
            rtts[i] = context.rtts[i];
        }

        free(context.rtts);

        context.rtts = rtts;

        insert_rtt(rtt);
    }
}

int dns_lookup() {
    struct addrinfo hint;
    struct addrinfo *res;

    memset(&hint, 0, sizeof(struct addrinfo));

    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_RAW;
    hint.ai_protocol = IPPROTO_ICMP;

    if (!getaddrinfo(context.requested_address, NULL, &hint, &res) && res->ai_addr) {
        memcpy(&context.resolved_address, res->ai_addr, sizeof(struct sockaddr_in));

        struct sockaddr_in *resolved_address = (struct sockaddr_in *)&context.resolved_address;

        inet_ntop(AF_INET, &resolved_address->sin_addr, context.numeric_resolved_address,
                  sizeof(context.numeric_resolved_address));

        context.numeric_resolved_address[INET_ADDRSTRLEN] = '\0';

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

    for (int i = 0; i < context.rtts_length; ++i) {
        deviation_sum += (context.rtts[i] - context.avg) * (context.rtts[i] - context.avg);
    }

    return sqrt(deviation_sum / context.rtts_length);
}

void initial_print() {
    printf("PING %s (%s) %d(%d) bytes of data\n", context.requested_address,
           context.numeric_resolved_address, context.data_size,
           (int)(context.data_size + sizeof(struct icmp)));
}

void final_print() {
    printf("\n--- %s ping statistics ---\n", context.requested_address);
    printf("%d packets transmitted, %d received, %.1f%% packet loss, time %.0fms\n",
           context.sent_packets, context.received_packets,
           (100.0f / ((float)context.sent_packets)) *
               (context.sent_packets - context.received_packets),
           context.time);

    if (!context.received_packets) {
        return;
    }

    printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n", context.min, context.avg, context.max,
           calculate_standard_deviation());
}

int ping() {
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

                insert_rtt(rtt);

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
                       bytes_received - ip_header_len, reverse_resolved_address,
                       context.numeric_resolved_address, context.sent_packets - 1, context.ttl,
                       rtt);
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

    final_print();

    free_context();

    exit(0);
}

int handle_flag(args_t *args, int argc, char **argv, int i) {
    char flag;

    flag = argv[i][1];

    if (flag == 'v') {
        args->verbose = 1;
        return (0);
    } else if (flag == 'g' && i + 1 < argc) {
        args->sweep_min = atoi(argv[i + 1]);
        return (1);
    } else if (flag == 'G' && i + 1 < argc) {
        args->sweep_max = atoi(argv[i + 1]);
        return (1);
    } else if (flag == 'h' && i + 1 < argc) {
        args->sweep_step = atoi(argv[i + 1]);
        return (1);
    }
    return (1000);
}

int handle_args(args_t *args, int argc, char **argv) {
    if (argc < 2) {
        return (NOT_ENOUGH_ARGUMENTS);
    }

    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            i += handle_flag(args, argc, argv, i);
        } else if (i + 1 == argc) {
            args->host = argv[i];
        } else {
            return (-1);
        }
    }

    if (!args->host) {
        return (-1);
    }

    return (0);
}

int main(int argc, char **argv) {
    // Mandatory:
    // v(verbose output)
    // ?(help)
    // f(flood)
    // i(interval between sending packets, < 0.2 for super user only)
    // l(preload, > 3 only for super user)
    // w(deadline)
    // W(timeout) wait time for reply packet
    // p(patter) specify patter for payload data
    //  r(bypass routing table) questionable
    // s(packet size) number of bytes as payload
    // t(ttl)
    //  T(timestamp) questionable
    if (init_context()) {
        return 0;
    }

    if (handle_arguments(argc, argv)) {
        return 0;
    }

    signal(SIGINT, int_handler);

    if (dns_lookup()) {
        printf("ping: %s: Name or service not known\n", context.requested_address);

        return 0;
    }

    initial_print();

    ping();

    final_print();

    free_context();

    return 0;
}
