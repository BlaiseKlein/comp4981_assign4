#include "../include/display.h"
#include "req_handler.h"
#include <fcntl.h>
#include <network.h>
#include <pthread.h>
#include <server.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    struct context *ctx;
    pid_t           pid;
    ctx = (struct context *)malloc(sizeof(struct context));
    parse_arguments(argc, argv, ctx);

    setup_socket(ctx);

    if(socketpair(AF_UNIX, SOCK_STREAM, 0, ctx->network.domain_fd) == -1)
    {
        perror("Error creating socket pair");
        exit(EXIT_FAILURE);
    }

    // Fork watcher, but don't pass ctx->network.domain_fd[0]
    fcntl(ctx->network.domain_fd[0], F_SETFD, fcntl(ctx->network.domain_fd[0], F_GETFD) | FD_CLOEXEC);
    pid = fork();
    if(pid == 0)
    {
        // If child: close ctx->network.domain_fd[0], start children, then start watcher procedure
        close(ctx->network.domain_fd[0]);
        setup_children(ctx);
        watch_children(ctx);
        free(ctx);
        exit(0);
    }
    else if(pid < 0)
    {
        // If error... fail
        free(ctx);
        perror("Error forking child");
        exit(EXIT_FAILURE);
    }
    else
    {
        // If parent: close ctx->network.domain_fd[1], start await connections
        close(ctx->network.domain_fd[1]);
        await_client_connection(ctx);    // TODO make select or poll, handle the client, the domain to children, and the currently working client sockets (keep alive)
        return 0;
    }

    free(ctx);
    return EXIT_SUCCESS;
}
