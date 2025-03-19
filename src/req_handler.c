//
// Created by blaise-klein on 3/6/25.
//

#include "req_handler.h"
#include "network.h"
#include <server.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int setup_children(struct context *ctx)
{
    for(int i = 0; i < CHILD_COUNT; ++i)
    {
        fork_child(ctx, i);
    }
    return 0;
}

_Noreturn void startup_child(struct context *ctx)
{
    // TODO add child functionality
    if(ctx->err > 0)
    {
        ctx->err = 0;
    }

    while(1)
    {
        char buf[1];
        int  err                    = 0;
        ctx->network.next_client_fd = recv_fd(ctx->network.domain_fd[1]);
        fprintf(stdout, "Client fd: %d\n", ctx->network.next_client_fd);
        send_fd(ctx->network.domain_fd[1], ctx->network.next_client_fd);
        read_fully(ctx->network.next_client_fd, buf, 1, &err);
        if(err < 0)
        {
            ctx->err = err;
        }
        close(ctx->network.next_client_fd);
    }
}
