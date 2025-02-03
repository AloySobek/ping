#include "ping.h"

void print_commence_message(struct sockinfo *sockinfo, struct config *config) {
    printf("PING %s (%s): %d data bytes\n", sockinfo->host, sockinfo->address, config->size);
}

void print_statistics(struct sockinfo *sockinfo, struct stats *stats) {
    printf("\n--- %s ping statistics ---\n", sockinfo->host);
    printf("%d packets transmitted, %d packets received, %.1f%% packet loss\n", stats->sent,
           stats->received, (100.0f / stats->sent * (stats->sent - stats->received)));
    printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n", stats->min, stats->avg,
           stats->max, 0.0f);
}

void print_ping(uint32_t n_bytes, char *host, uint32_t sequence, uint32_t ttl, float ms) {
    printf("%d bytes from %s: icmp_seq=%d ttl=%d, time=%.1f ms\n", n_bytes, host, sequence, ttl,
           ms);
}

void print_icmp_err(int type, int code) {
    switch (type) {
    case ICMP_DEST_UNREACH:
        switch (code) {
        case ICMP_NET_UNREACH:
            printf("Destination Net Unreachable\n");
            break;
        case ICMP_HOST_UNREACH:
            printf("Destination Host Unreachable\n");
            break;
        case ICMP_PROT_UNREACH:
            printf("Destination Protocol Unreachable\n");
            break;
        case ICMP_PORT_UNREACH:
            printf("Destination Port Unreachable\n");
            break;
        case ICMP_FRAG_NEEDED:
            printf("Frag needed\n");
            break;
        case ICMP_SR_FAILED:
            printf("Source Route Failed\n");
            break;
        case ICMP_NET_UNKNOWN:
            printf("Destination Net Unknown\n");
            break;

        case ICMP_HOST_UNKNOWN:
            printf("Destination Host Unknown\n");

            break;
        case ICMP_HOST_ISOLATED:
            printf("Source Host Isolated\n");
            break;
        case ICMP_NET_ANO:
            printf("Destination Net Prohibited\n");
            break;
        case ICMP_HOST_ANO:

            printf("Destination Host Prohibited\n");
            break;
        case ICMP_NET_UNR_TOS:
            printf("Destination Net Unreachable for Type of Service\n");

            break;
        case ICMP_HOST_UNR_TOS:
            printf("Destination Host Unreachable for Type of Service\n");
            break;
        case ICMP_PKT_FILTERED:

            printf("Packet filtered\n");
            break;
        case ICMP_PREC_VIOLATION:
            printf("Precedence Violation\n");
            break;
        case ICMP_PREC_CUTOFF:
            printf("Precedence Cutoff\n");
            break;
        default:
            printf("Dest Unreachable, Bad Code: %d\n", code);
            break;
        }
        break;
    case ICMP_SOURCE_QUENCH:
        printf("Source Quench\n");
        break;
    case ICMP_REDIRECT:
        switch (code) {
        case ICMP_REDIR_NET:
            printf("Redirect Network");
            break;
        case ICMP_REDIR_HOST:
            printf("Redirect Host");

            break;
        case ICMP_REDIR_NETTOS:
            printf("Redirect Type of Service and Network");
            break;
        case ICMP_REDIR_HOSTTOS:
            printf("Redirect Type of Service and Host");
            break;
        default:
            printf("Redirect, Bad Code: %d", code);
            break;
        }

        break;
    }
}
