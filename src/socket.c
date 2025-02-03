#include "ping.h"

_Bool dns_lookup(struct sockinfo *sockinfo) {
    struct addrinfo hint = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_RAW,
        .ai_protocol = IPPROTO_ICMP,
    };
    struct addrinfo *res = NULL;

    int err;

    if ((err = getaddrinfo(sockinfo->host, NULL, &hint, &res)) != 0) {
        printf("ft_ping: dns resolution error: %s", gai_strerror(err));

        return 1;
    }

    sockinfo->internet_address = *(struct sockaddr_in *)res->ai_addr;

    if (inet_ntop(AF_INET, &sockinfo->internet_address.sin_addr, sockinfo->address,
                  INET_ADDRSTRLEN) == NULL) {
        printf("ft_ping: address translation error: %s", strerror(errno));

        return 1;
    }

    freeaddrinfo(res);

    return 0;
}

_Bool create_and_configure_socket(struct sockinfo *sockinfo, struct config *config) {
    if ((sockinfo->socketfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
        printf("ft_ping: socket creation error: %s", strerror(errno));

        return 1;
    }

    if (setsockopt(sockinfo->socketfd, IPPROTO_IP, IP_TTL, &config->ttl, sizeof(config->ttl)) !=
        0) {
        printf("ft_ping: socket configuring error: %s", strerror(errno));

        close(sockinfo->socketfd);

        return 1;
    }

    return 0;
}

_Bool destroy_socket(struct sockinfo *sockinfo) {
    if (close(sockinfo->socketfd) != 0) {
        return 1;
    }

    return 0;
}
