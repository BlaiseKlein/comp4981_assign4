#include "../include/display.h"
#include "req_handler.h"
#include <network.h>
#include <server.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    struct context *ctx;

    ctx = (struct context *)malloc(sizeof(struct context));
    parse_arguments(argc, argv, ctx);

    setup_socket(ctx);

    setup_domain_socket(ctx);
    ctx->network.domain_fd = connect_to_server(ctx->network.receive_addr, ctx->network.receive_addr_len, &ctx->err);

    setup_children(ctx);

    await_client_connection(ctx);

    free(ctx);
    return EXIT_SUCCESS;
}
