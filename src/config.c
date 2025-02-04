#include "ping.h"

static _Bool parse_cli(int argc, char **argv, struct config *config) {
    if (config == NULL) {
        return 1;
    }

    int option;

    while ((option = getopt(argc, argv, "vht:s:?")) != -1) {
        switch (option) {
        case 'v': {
            config->flags |= FLAG_VERBOSE;

            break;
        }
        case 'h': {
            config->flags |= FLAG_HELP;

            break;
        }
        case 't': {
            config->ttl = atoi(optarg);

            break;
        }
        case 's': {
            config->size = atoi(optarg);

            break;
        }
        case '?': {
            config->flags = FLAG_HELP;

            break;
        }
        default: {
            return 1;

            break;
        }
        }
    }

    if (optind < argc) {
        config->host = argv[optind];
    }

    return 0;
};

static void print_usage() {
    printf("Usage: ft_ping [OPTION...] HOST ...\n"
           "Send ICMP ECHO_REQUEST packets to network hosts\n\n"
           "\t-v - Verbose outpout\n"
           "\t-h - Print this message\n"
           "\t-t [short] - TTL(Time To Live) for socket\n"
           "\t-s [char] - Payload size\n");
}

_Bool init_config_from_cli(int argc, char **argv, struct config *config) {
    if (getuid() != 0) {
        printf("ft_ping: usage error: root privileges required\n");

        return 1;
    }

    if (parse_cli(argc, argv, config) != 0) {
        print_usage();

        return 1;
    }

    if (config->flags & FLAG_HELP) {
        print_usage();

        return 1;
    }

    if (config->host == NULL) {
        printf("ft_ping: missing host operand\nTry 'ft_ping' -? for more information\n");

        return 1;
    }

    return 0;
}
