//
// Created by blaise-klein on 3/4/25.
//

#include "server.h"
#include "data_types.h"
#include <arpa/inet.h>    // for inet_pton
#include <p101_posix/p101_unistd.h>
#include <req_handler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define UNKNOWN_OPTION_MESSAGE_LEN 24
#define MAXARGS 5
#define PORTBASE 10
#define MAXPORT 65535

_Noreturn void usage(const char *program_name, int exit_code, const char *message)
{
    fprintf(stderr, "%s: %d, %s\n", program_name, exit_code, message);
    fprintf(stderr, "Usage: server [-i ip_address] [-p port]\n");
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
                struct in_addr  ipv4_addr;
                struct in6_addr ipv6_addr;

                ctx->arg.sys_addr     = optarg;
                ctx->arg.sys_addr_len = (ssize_t)strlen(optarg);

                if(inet_pton(AF_INET, ctx->arg.sys_addr, &ipv4_addr) != 1 && inet_pton(AF_INET6, ctx->arg.sys_addr, &ipv6_addr) != 1)
                {
                    usage(argv[0], EXIT_FAILURE, "Invalid IP address format.");
                }
                break;
            }
            case 'p':
            {
                char *endptr;
                long  port = strtol(optarg, &endptr, PORTBASE);

                ctx->arg.sys_port = optarg;

                // Check if the input is all digits
                for(size_t i = 0; i < strlen(optarg); i++)
                {
                    if(optarg[i] < '0' || optarg[i] > '9')
                    {
                        usage(argv[0], EXIT_FAILURE, "Port must be a number between 1 and 65535.");
                    }
                }

                if(port < 1 || port > MAXPORT)
                {
                    usage(argv[0], EXIT_FAILURE, "Port out of range. Must be between 1 and 65535.");
                }

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
        // fprintf(stderr, "%d\n", argc);
        usage(argv[0], EXIT_FAILURE, "Wrong number of arguments.");
    }
}

_Noreturn void *watch_children(void *arg)
{
    struct context *ctx = (struct context *)arg;
    // printf("[watch_children DEBUG] Start\n");

    while(1)
    {
        for(int i = 0; i < CHILD_COUNT; i++)
        {
            int   status;
            pid_t result = waitpid(ctx->network.child_pids[i], &status, WNOHANG);
            if(result == 0)
            {
                continue;
            }
            if(result == -1)
            {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
            else
            {
                printf("Restarting child\n");
                fork_child(ctx, i);
                // Child exited
            }
        }
    }
}

int fork_child(struct context *ctx, int child_pid)
{
    int pid = fork();
    // printf("[fork_child DEBUG] Start\n");

    if(pid == -1)
    {
        perror("fork");
        return -1;
    }
    if(pid == 0)
    {
        startup_child(ctx);
    }
    else
    {
        ctx->network.child_pids[child_pid] = pid;
    }
    return 0;
}
