#include "ping.h"

int handle_flag(args_t *args, int argc, char **argv, int i)
{
    char flag;

    flag = argv[i][1];

    if (flag == 'v')
    {
        args->verbose = 1;
        return (0);
    }
    else if (flag == 'g' && i + 1 < argc)
    {
        args->sweep_min = atoi(argv[i + 1]);
        return (1);
    }
    else if (flag == 'G' && i + 1 < argc)
    {
        args->sweep_max = atoi(argv[i + 1]);
        return (1);
    }
    else if (flag == 'h' && i + 1 < argc)
    {
        args->sweep_step = atoi(argv[i + 1]);
        return (1);
    }
    return (1000);
}

int handle_args(args_t *args, int argc, char **argv)
{
    if (argc < 2)
    {
        return (NOT_ENOUGH_ARGUMENTS);
    }

    for (int i = 1; i < argc; ++i)
    {
        if (argv[i][0] == '-')
        {
            i += handle_flag(args, argc, argv, i);
        }
        else if (i + 1 == argc)
        {
            args->host = argv[i];
        }
        else
        {
            return (-1);
        }
    }

    if (!args->host)
    {
        return (-1);
    }

    return (0);
}
