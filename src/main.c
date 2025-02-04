#include "ping.h"

_Bool _running = 1;
_Bool _should_send = 1;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        _running = 0;
    } else if (signal == SIGALRM) {
        _should_send = 1;
    }
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

double calculate_standard_deviation(struct Node *list, float avg) {
    long double deviation_sum = 0.0f;
    uint32_t n = 0;

    for (struct Node *iter = list; iter; iter = iter->next) {
        deviation_sum += (iter->x - avg) * (iter->x - avg);

        n += 1;
    }

    return sqrtf(deviation_sum / (long double)n);
}

_Bool send_icmp_segment(struct sockinfo *sockinfo, struct config *config, struct stats *stats) {
    static uint32_t sequence = 0;

    uint8_t icmp_segment[sizeof(struct icmphdr) + config->size] = {};

    struct timespec *timestamp = (void *)(icmp_segment + sizeof(struct icmphdr));

    if (clock_gettime(CLOCK_REALTIME, timestamp) != 0) {
        printf("ft_ping: clock_gettime error: %s", strerror(errno));

        return 1;
    }

    ((struct icmphdr *)icmp_segment)->type = ICMP_ECHO;
    ((struct icmphdr *)icmp_segment)->un.echo.id = getpid();
    ((struct icmphdr *)icmp_segment)->un.echo.sequence = sequence;
    ((struct icmphdr *)icmp_segment)->checksum =
        checksum((void *)icmp_segment, sizeof(icmp_segment));

    sequence += 1;

    // sendto takes the buffer and fills the ip header itself, which is not how recvfrom works,
    // must be careful here
    if (sendto(sockinfo->socketfd, icmp_segment, sizeof(icmp_segment), 0,
               (struct sockaddr *)&sockinfo->internet_address,
               sizeof(sockinfo->internet_address)) <= 0) {
        printf("ft_ping: icmp segment sending error: %s", strerror(errno));

        return 1;
    }

    stats->sent += 1;

    return 0;
}

_Bool receive_icmp_segment(struct sockinfo *sockinfo, struct config *config, struct stats *stats) {
    uint8_t buf[2048];

    struct iovec iovec = {.iov_base = buf, .iov_len = sizeof(buf)};
    struct msghdr msghdr = {.msg_iov = &iovec, .msg_iovlen = 1};

    ssize_t n_bytes;

    if ((n_bytes = recvmsg(sockinfo->socketfd, &msghdr, MSG_DONTWAIT)) == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }

        printf("ft_ping: recvmsg error: %s", strerror(errno));

        return 1;
    }

    struct in_addr src = {.s_addr = ((struct iphdr *)buf)->saddr};

    if (((struct icmphdr *)(buf + sizeof(struct iphdr)))->type == ICMP_ECHOREPLY) {
        if (((struct icmphdr *)(buf + sizeof(struct iphdr)))->un.echo.id == getpid()) {
            struct timespec now;
            struct timespec *timestamp =
                (void *)(buf + sizeof(struct iphdr) + sizeof(struct icmphdr));

            if (clock_gettime(CLOCK_REALTIME, &now) != 0) {
                printf("ft_ping: clock_gettime error: %s", strerror(errno));

                return 1;
            }

            stats->received += 1;

            print_ping(n_bytes - sizeof(struct iphdr), inet_ntoa(src),
                       ((struct icmphdr *)(buf + sizeof(struct iphdr)))->un.echo.sequence,
                       ((struct iphdr *)buf)->ttl, (now.tv_nsec - timestamp->tv_nsec) / 1000000.0f);

            stats->list =
                list_prepend(stats->list, (now.tv_nsec - timestamp->tv_nsec) / 1000000.0f);
        }
    } else if (((struct icmphdr *)(buf + sizeof(struct iphdr)))->type != ICMP_ECHO) {
        uint8_t *err_buf =
            buf + sizeof(struct iphdr) + sizeof(struct icmphdr) + sizeof(struct iphdr);

        if (((struct icmphdr *)err_buf)->un.echo.id == getpid()) {
            print_icmp_err(n_bytes - sizeof(struct iphdr), inet_ntoa(src),
                           ((struct icmphdr *)(buf + sizeof(struct iphdr)))->type,
                           ((struct icmphdr *)(buf + sizeof(struct iphdr)))->code,
                           (config->flags & FLAG_VERBOSE ? 1 : 0),
                           (config->flags & FLAG_VERBOSE ? (struct iphdr *)buf : NULL),
                           ((struct icmphdr *)(err_buf)), n_bytes);
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    struct config config = {.size = 56, .ttl = 64};

    if (init_config_from_cli(argc, argv, &config) != 0) {
        return -1;
    }

    signal(SIGINT, signal_handler);
    signal(SIGALRM, signal_handler);

    struct sockinfo sockinfo = {.host = config.host};

    if (dns_lookup(&sockinfo) != 0) {
        return -1;
    }

    if (create_and_configure_socket(&sockinfo, &config) != 0) {
        return -1;
    }

    struct stats stats = {0};

    print_commence_message(&sockinfo, &config);

    _Bool err = 0;

    while (_running) {
        if (_should_send) {
            _should_send = 0;

            if (send_icmp_segment(&sockinfo, &config, &stats) != 0) {
                err = 1;

                break;
            }

            alarm(1);
        }

        if (receive_icmp_segment(&sockinfo, &config, &stats) != 0) {
            err = 1;

            break;
        }
    }

    if (!err) {
        print_statistics(&sockinfo, &stats);
    }

    list_free(stats.list);
    destroy_socket(&sockinfo);

    return 0;
}
