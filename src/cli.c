#include "ping.h"

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
// T(timestamp) questionable

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

int handle_arguments(int argc, char **argv) {
    if (argc < 2) {
        printf("ping: usage error: Destination address required\n");

        return -1;
    }

    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            int len = strlen(argv[i]);

            for (int j = 1; j < len; ++j) {
                char flag = argv[i][j];

                switch (flag) {
                case 'V': {
                    printf("Printing version and exiting...\n");
                    exit(0);
                }
                case 'c': {
                    break;
                }
                default: {
                    break;
                }
                }
            }
            // Handle flag
        } else {
            context.requested_address = argv[i];
        }
    }

    return 0;
}
