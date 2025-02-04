#include "ping.h"

void print_commence_message(struct sockinfo *sockinfo, struct config *config) {
    char buf[256];
    int n;

    n = sprintf(buf, ", id 0x%x = %d", getpid(), getpid());
    buf[n] = '\0';

    printf("PING %s (%s): %d data bytes%s\n", sockinfo->host, sockinfo->address, config->size,
           (config->flags & FLAG_VERBOSE ? buf : ""));
}

void print_statistics(struct sockinfo *sockinfo, struct stats *stats) {
    float min = FLT_MAX;
    float avg = 0.0f;
    float max = FLT_MIN;

    long double sum = 0.0f;

    uint32_t n = 0;

    for (struct Node *iter = stats->list; iter; iter = iter->next) {
        if (iter->x < min) {
            min = iter->x;
        }

        if (iter->x > max) {
            max = iter->x;
        }

        sum += iter->x;
        n += 1;
    }

    avg = sum / (long double)n;

    printf("--- %s ping statistics ---\n", sockinfo->host);
    printf("%d packets transmitted, %d packets received, %.1f%% packet loss\n", stats->sent,
           stats->received, (100.0f / stats->sent * (stats->sent - stats->received)));
    if (stats->list) {
        printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n", min, avg, max,
               calculate_standard_deviation(stats->list, avg));
    }
}

void print_ping(uint32_t n_bytes, char *host, uint32_t sequence, uint32_t ttl, float ms) {
    printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n", n_bytes, host, sequence, ttl, ms);
}

void print_headers_dump(struct iphdr *ip, struct icmphdr *icmp, int size) {
    // Convert IP addresses to string format
    char src_ip[INET_ADDRSTRLEN], dst_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip->saddr, src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &ip->daddr, dst_ip, INET_ADDRSTRLEN);

    // Print IP Header Dump in hexadecimal
    uint16_t *ip_raw = (uint16_t *)ip;
    printf("IP Hdr Dump:\n");
    for (unsigned long i = 0; i < sizeof(struct iphdr) / 2; i++) {
        printf("%04x ", ntohs(ip_raw[i]));
    }
    printf("\n");

    // Print IP header details
    printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst     Data\n");
    printf(" %d  %d  %02x %04x %04x   %d %04x  %02x  %02x %04x %s  %s\n", ip->version, ip->ihl,
           ip->tos, ip->tot_len, ip->id, (ip->frag_off >> 13),
           (ip->frag_off & 0x1FFF), ip->ttl, ip->protocol, ip->check, src_ip, dst_ip);

    // Print ICMP header details
    printf("ICMP: type %d, code %d, size %d, id 0x%04x, seq 0x%04x\n", icmp->type, icmp->code, size,
           icmp->un.echo.id, icmp->un.echo.sequence);
}

void print_icmp_err(uint32_t n_bytes, char *address, int type, int code, _Bool verbose,
                    struct iphdr *iphdr, struct icmphdr *icmphdr, int size) {
    char buf[256];
    int n = 0;

    switch (type) {
    case ICMP_DEST_UNREACH: {
        n = sprintf(buf, "Destination unreachable");

        break;
    }
    case ICMP_SOURCE_QUENCH: {
        n = sprintf(buf, "Source quench");

        break;
    }
    case ICMP_REDIRECT: {
        n = sprintf(buf, "Redirect");

        break;
    }
    case ICMP_TIME_EXCEEDED: {
        switch (code) {
        case ICMP_EXC_TTL: {
            n = sprintf(buf, "Time to live exceeded");

            break;
        }
        case ICMP_EXC_FRAGTIME: {
            n = sprintf(buf, "Time exceeded due to fragtime");

            break;
        }
        }

        break;
    }
    case ICMP_PARAMETERPROB: {
        n = sprintf(buf, "Parameter prob");

        break;
    }
    }

    buf[n] = '\0';

    printf("%d bytes from %s: %s\n", n_bytes, address, buf);

    if (verbose) {
        print_headers_dump(iphdr, icmphdr, size);
    }
}
