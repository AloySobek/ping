#include "ping.h"

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
