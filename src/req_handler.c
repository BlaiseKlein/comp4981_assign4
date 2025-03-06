//
// Created by blaise-klein on 3/6/25.
//

#include "req_handler.h"
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

int setup_children(struct context *ctx)
{
    for(int i = 0; i < CHILD_COUNT; ++i)
    {
        int pid = fork();
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
            ctx->network.child_pids[i] = pid;
        }
    }
    return 0;
}

void startup_child(struct context *ctx)
{
    if(ctx->err > 0)
    {
        ctx->err = 0;
    }
}