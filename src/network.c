//
// Created by blaise-klein on 2/18/25.
//

#include "network.h"
#include "data_types.h"
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#define BASE_TEN 10

static void send_fd(int socket, int fd)
{
    struct msghdr   msg = {0};
    struct iovec    io;
    char            buf[1] = {0};
    struct cmsghdr *cmsg;
    char            control[CMSG_SPACE(sizeof(int))];
    io.iov_base        = buf;
    io.iov_len         = sizeof(buf);
    msg.msg_iov        = &io;
    msg.msg_iovlen     = 1;
    msg.msg_control    = control;
    msg.msg_controllen = sizeof(control);
    cmsg               = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level   = SOL_SOCKET;
    cmsg->cmsg_type    = SCM_RIGHTS;
    cmsg->cmsg_len     = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &fd, sizeof(int));
    if(sendmsg(socket, &msg, 0) < 0)
    {
        perror("sendmsg");
        exit(EXIT_FAILURE);
    }
}

ssize_t read_fully(int sockfd, void *buf, ssize_t len, int *err)
{
    ssize_t total_read = 0;

    while(total_read < len)
    {
        ssize_t bytes_read;

        bytes_read = read(sockfd, (char *)buf + total_read, (size_t)(len - total_read));
        if(bytes_read == -1)
        {
            perror("read_fully");
            *err = -1;
            return 0;
        }

        total_read += bytes_read;
    }

    return total_read;
}

ssize_t write_fully(int sockfd, const void *buf, ssize_t len, int *err)
{
    ssize_t total_written = 0;

    while(total_written < len)
    {
        ssize_t bytes_written;

        bytes_written = write(sockfd, (const char *)buf + total_written, (size_t)(len - total_written));
        if(bytes_written == -1)
        {
            perror("write_fully");
            *err = -1;
            return 0;
        }

        total_written += bytes_written;
    }

    return total_written;
}

in_port_t parse_in_port_t(const char *str, int *err)
{
    char *endptr;
    long  parsed_value;
    *err = 0;

    parsed_value = strtol(str, &endptr, BASE_TEN);

    if(errno != 0)
    {
        *err = -3;
    }

    // Check if there are any non-numeric characters in the input string
    if(*endptr != '\0')
    {
        // usage(binary_name, EXIT_FAILURE, "Invalid characters in input.");
        *err = -1;
    }

    // Check if the parsed value is within the valid range for in_port_t
    if(parsed_value > UINT16_MAX)
    {
        // usage(binary_name, EXIT_FAILURE, "in_port_t value out of range.");
        *err = -2;
    }

    return (in_port_t)parsed_value;
}

void convert_address(const char *address, struct sockaddr_storage *addr, socklen_t *addr_len, int *err)
{
    memset(addr, 0, sizeof(*addr));

    if(inet_pton(AF_INET, address, &(((struct sockaddr_in *)addr)->sin_addr)) == 1)
    {
        addr->ss_family = AF_INET;
        *addr_len       = sizeof(struct sockaddr_in);
    }
    else if(inet_pton(AF_INET6, address, &(((struct sockaddr_in6 *)addr)->sin6_addr)) == 1)
    {
        addr->ss_family = AF_INET6;
        *addr_len       = sizeof(struct sockaddr_in6);
    }
    else
    {
        *err = -1;
    }
}

int socket_create(int domain, int type, int protocol, int *err)
{
    int sockfd;

    sockfd = socket(domain, type, protocol);

    if(sockfd == -1)
    {
        *err = -1;
    }

    return sockfd;
}

void socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port, int *err)
{
    char      addr_str[INET6_ADDRSTRLEN];
    socklen_t addr_len;
    void     *vaddr;
    in_port_t net_port;

    net_port = htons(port);

    if(addr->ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4_addr;

        ipv4_addr           = (struct sockaddr_in *)addr;
        addr_len            = sizeof(*ipv4_addr);
        ipv4_addr->sin_port = net_port;
        vaddr               = (void *)&(((struct sockaddr_in *)addr)->sin_addr);
    }
    else if(addr->ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr            = (struct sockaddr_in6 *)addr;
        addr_len             = sizeof(*ipv6_addr);
        ipv6_addr->sin6_port = net_port;
        vaddr                = (void *)&(((struct sockaddr_in6 *)addr)->sin6_addr);
    }
    else
    {
        *err = -1;
        return;
    }

    if(inet_ntop(addr->ss_family, vaddr, addr_str, sizeof(addr_str)) == NULL)
    {
        *err = -2;
        return;
    }

    printf("Binding to: %s:%u\n", addr_str, port);

    if(sockfd >= 0)
    {
        int bind_result;
        int listen_result;
        bind_result = bind(sockfd, (struct sockaddr *)addr, addr_len);
        if(bind_result < 0)
        {
            perror("Binding failed");
            fprintf(stderr, "Error code: %d\n", errno);
            *err = -3;
            return;
        }
        printf("Bound to socket: %s:%u\n", addr_str, port);

        listen_result = listen(sockfd, SOMAXCONN);
        if(listen_result < 0)
        {
            perror("Listening failed");
            fprintf(stderr, "Error code: %d\n", errno);
            *err = -4;
            return;
        }
    }    // TODO FINISH
    else
    {
        *err = -1;
    }
}

void socket_close(int sockfd)
{
    if(close(sockfd) == -1)
    {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}

void get_address_to_server(struct sockaddr_storage *addr, in_port_t port, int *err)
{
    if(addr->ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4_addr;

        ipv4_addr             = (struct sockaddr_in *)addr;
        ipv4_addr->sin_family = AF_INET;
        ipv4_addr->sin_port   = htons(port);
    }
    else if(addr->ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr              = (struct sockaddr_in6 *)addr;
        ipv6_addr->sin6_family = AF_INET6;
        ipv6_addr->sin6_port   = htons(port);
    }
    else
    {
        *err = -1;
    }
}

int setup_socket(struct context *ctx)
{
    int sockfd;
    // int listen_return;
    ctx->network.receive_addr = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
    if(ctx->network.receive_addr == NULL)
    {
        return -1;
    }

    ctx->network.receive_port = parse_in_port_t(ctx->arg.sys_port, &ctx->err);
    if(ctx->err < 0)
    {
        free(ctx->network.receive_addr);
        return -1;
    }
    convert_address(ctx->arg.sys_addr, ctx->network.receive_addr, &ctx->network.receive_addr_len, &ctx->err);
    if(ctx->err < 0)
    {
        free(ctx->network.receive_addr);
        return -1;
    }
    sockfd = socket_create(ctx->network.receive_addr->ss_family, SOCK_STREAM, 0, &ctx->err);
    if(ctx->err < 0)
    {
        close(sockfd);
        perror("Create failed");
        free(ctx->network.receive_addr);
        return -1;
    }
    socket_bind(sockfd, ctx->network.receive_addr, ctx->network.receive_port, &ctx->err);
    if(ctx->err < 0)
    {
        close(sockfd);
        free(ctx->network.receive_addr);
        return -1;
    }

    // listen_return = listen(sockfd, 0);
    // if(listen_return < 0)
    // {
    //     perror("Listen failed");
    //     close(sockfd);
    //     return -1;
    // }

    ctx->network.receive_fd = sockfd;
    printf("Listening for incoming connections...\n");
    return 0;
}

int setup_domain_socket(struct context *ctx)
{
    struct sockaddr_un *un_addr;

    memset(ctx->network.receive_addr, 0, sizeof(*ctx->network.receive_addr));
    un_addr                              = (struct sockaddr_un *)ctx->network.receive_addr;
    ctx->network.receive_addr->ss_family = AF_UNIX;
    strncpy(un_addr->sun_path, DOMAINSOCKET, sizeof(un_addr->sun_path) - 1);
    if(errno != 0)
    {
        return -1;
    }
    un_addr->sun_path[sizeof(un_addr->sun_path) - 1] = '\0';
    ctx->network.receive_addr_len                    = sizeof(struct sockaddr_un);
    return 0;
}

int await_client_connection(struct context *ctx)
{
    while(1)
    {
        int result;
        result = accept(ctx->network.receive_fd, (struct sockaddr *)ctx->network.receive_addr, &ctx->network.receive_addr_len);
        if(result < 0)
        {
            printf("%d\n", result);
            printf("%d\n", errno);
            close(ctx->network.receive_fd);
            return -1;
        }

        ctx->network.next_client_fd = result;
        result                      = forward_request(ctx);
        if(result < 0)
        {
            close(ctx->network.receive_fd);
            return -1;
        }
    }
}

int forward_request(const struct context *ctx)
{
    struct msghdr   msg = {0};
    struct iovec    io;
    char            buf[1] = {0};
    struct cmsghdr *cmsg;
    char            control[CMSG_SPACE(sizeof(int))];
    io.iov_base        = buf;
    io.iov_len         = sizeof(buf);
    msg.msg_iov        = &io;
    msg.msg_iovlen     = 1;
    msg.msg_control    = control;
    msg.msg_controllen = sizeof(control);
    cmsg               = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level   = SOL_SOCKET;
    cmsg->cmsg_type    = SCM_RIGHTS;
    cmsg->cmsg_len     = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &ctx->network.next_client_fd, sizeof(int));
    if(sendmsg(ctx->network.domain_fd, &msg, 0) < 0)
    {
        perror("sendmsg");
        return -1;
    }
    return 0;
}
