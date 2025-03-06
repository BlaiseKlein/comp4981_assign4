//
// Created by blaise-klein on 3/4/25.
//

#include "server.h"
#include "data_types.h"
#include <p101_posix/p101_unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define UNKNOWN_OPTION_MESSAGE_LEN 24
#define MAXARGS 5

_Noreturn void usage(const char *program_name, int exit_code, const char *message)
{
    fprintf(stderr, "%s: %d, %s\n", program_name, exit_code, message);
    exit(EXIT_FAILURE);
}

void parse_arguments(int argc, char **argv, struct context *ctx)
{
    int opt;

    opterr = 0;

    while((opt = getopt(argc, argv, "i:p:h")) != -1)
    {
        switch(opt)
        {
            case 'i':
            {
                ctx->arg.sys_addr     = optarg;
                ctx->arg.sys_addr_len = (ssize_t)strlen(optarg);
                break;
            }
            case 'p':
            {
                ctx->arg.sys_port = optarg;
                break;
            }
            case 'h':
            {
                usage(argv[0], EXIT_SUCCESS, "Usage: [-i sys_addr] [-p sys_port]");
            }
            case '?':
            {
                if(optopt == 'c')
                {
                    usage(argv[0], EXIT_FAILURE, "Option '-c' requires a value.");
                }
                else
                {
                    char message[UNKNOWN_OPTION_MESSAGE_LEN];

                    snprintf(message, sizeof(message), "Unknown option '-%c'.", optopt);
                    usage(argv[0], EXIT_FAILURE, message);
                }
            }
            default:
            {
                usage(argv[0], EXIT_FAILURE, "Usage: [-i sys_addr] [-p sys_port]");
            }
        }
    }

    if(optind < argc)
    {
        usage(argv[0], EXIT_FAILURE, "Too many arguments.");
    }
    if(argc != 2 && argc != MAXARGS)
    {
        usage(argv[0], EXIT_FAILURE, "Wrong number of arguments.");
    }
}
