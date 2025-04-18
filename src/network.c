//
// Created by blaise-klein on 2/18/25.
//

#include "network.h"
#include "data_types.h"
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#define BASE_TEN 10
#define REQUIRED_SOCKET_COUNT 2

void send_fd(int socket, int fd)
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
    // printf("[send_fd DEBUG] Start\n");

    if(sendmsg(socket, &msg, 0) < 0)
    {
        perror("sendmsg");
        exit(EXIT_FAILURE);
    }
}

ssize_t read_fully(int sockfd, void *buf, ssize_t len, int *err)
{
    ssize_t total_read = 0;
    // printf("[read_fully DEBUG] Start\n");

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
    // printf("[write_fully DEBUG] Start\n");

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
    // printf("[parse_in_port_t DEBUG] Start\n");

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
    // printf("[convert_address DEBUG] Start\n");

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
    // printf("[socket_create DEBUG] Start\n");

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
    // printf("[socket_bind DEBUG] Start\n");

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
    // printf("[socket_close DEBUG] Start\n");

    if(close(sockfd) == -1)
    {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}

void get_address_to_server(struct sockaddr_storage *addr, in_port_t port, int *err)
{
    // printf("[get_address_to_server DEBUG] Start\n");

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
    // printf("[setup_socket DEBUG] Start\n");

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
    // printf("[setup_domain_socket DEBUG] Start\n");

    if(socketpair(AF_UNIX, SOCK_STREAM, 0, ctx->network.domain_fd) == -1)
    {
        perror("Error creating socket pair");
        ctx->err = -1;
        return -1;
    }

    return 0;
}

_Noreturn int await_client_connection(struct context *ctx)
{
    nfds_t max_clients    = 0;
    ctx->network.poll_fds = initialize_pollfds(ctx->network.receive_fd, ctx->network.domain_fd[0], &ctx->network.poll_clients);
    //    printf("[await_client_connection DEBUG] Start\n");

    while(1)
    {
        int activity = poll(ctx->network.poll_fds, max_clients + REQUIRED_SOCKET_COUNT, -1);

        if(activity < 0)
        {
            perror("Poll error");
            exit(EXIT_FAILURE);
        }

        if(ctx->network.poll_fds[0].fd != -1 && (ctx->network.poll_fds[0].revents & POLLIN))
        {
            handle_new_connection(ctx->network.receive_fd, &ctx->network.poll_clients, &max_clients, &ctx->network.poll_fds);
        }

        if(ctx->network.poll_fds[1].fd != -1 && (ctx->network.poll_fds[1].revents & POLLIN))
        {
            //            int         result;
            struct stat my_fd;
            struct stat target_fd;
            int         new_fd = recv_fd(ctx->network.poll_fds[1].fd);
            if(new_fd < 0)
            {
                perror("Read error");
                exit(EXIT_FAILURE);
            }

            if(fstat(new_fd, &target_fd) < 0)
            {
                perror("Fstat error");
                exit(EXIT_FAILURE);
            }

            for(nfds_t i = REQUIRED_SOCKET_COUNT; i < max_clients + REQUIRED_SOCKET_COUNT; i++)
            {
                if(fstat(ctx->network.poll_fds[i].fd, &my_fd) < 0)
                {
                    perror("Fstat error");
                    exit(EXIT_FAILURE);
                }

                if(my_fd.st_ino == target_fd.st_ino)
                {
#ifdef __linux
                    ctx->network.poll_fds[i].events = POLLIN | POLLRDHUP | POLLHUP;
#endif
#ifdef __APPLE__
                    ctx->network.poll_fds[i].events = POLLIN | POLLHUP;
#endif
                    close(new_fd);
                    break;
                }
            }
        }

        for(nfds_t i = REQUIRED_SOCKET_COUNT; i < max_clients + REQUIRED_SOCKET_COUNT; i++)
        {
            ssize_t result;
            char    buf[1];

            if(ctx->network.poll_fds[i].fd != -1 && (ctx->network.poll_fds[i].revents & POLLIN))
            {
                send_fd(ctx->network.domain_fd[0], ctx->network.poll_fds[i].fd);
                ctx->network.poll_fds[i].events  = 0;
                ctx->network.poll_fds[i].revents = 0;
            }

#ifdef __linux
            if(ctx->network.poll_fds[i].fd != -1 && (ctx->network.poll_fds[i].revents & POLLRDHUP))
            {
                ctx->network.poll_fds[i].events = 0;
                fprintf(stderr, "[await_client_connection DEBUG] Considering removal: fd=%d, revents=0x%x\n", ctx->network.poll_fds[i].fd, (unsigned int)ctx->network.poll_fds[i].revents);

                remove_poll_client((int)i, &ctx->network.poll_clients, &max_clients, &ctx->network.poll_fds);
                ctx->network.poll_fds[i].revents = 0;
                break;
            }
#endif

#ifdef __APPLE__
            if(ctx->network.poll_fds[i].fd != -1 && (ctx->network.poll_fds[i].revents & POLLHUP))
            {
                result = recv(ctx->network.poll_fds[i].fd, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
                if(result == 0 || (result == -1 && (errno == ECONNRESET || errno == ENOTCONN)))
                {
                    ctx->network.poll_fds[i].events = 0;
                    fprintf(stderr, "[await_client_connection DEBUG] Removing dead fd=%d, revents=0x%x\n", ctx->network.poll_fds[i].fd, (unsigned int)ctx->network.poll_fds[i].revents);

                    remove_poll_client((int)i, &ctx->network.poll_clients, &max_clients, &ctx->network.poll_fds);
                    ctx->network.poll_fds[i].revents = 0;
                    break;
                }

                {
                    fprintf(stderr, "[await_client_connection DEBUG] Ignoring early POLLHUP on fd=%d\n", ctx->network.poll_fds[i].fd);
                    ctx->network.poll_fds[i].revents = 0;
                    continue;
                }
            }
#endif

            if(ctx->network.poll_fds[i].fd != -1)
            {
                result = recv(ctx->network.poll_fds[i].fd, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
                if(((result == -1 && (errno == ECONNRESET || errno == ENOTCONN)) || result == 0) && ctx->network.poll_fds[i].events & POLLIN)
                {
                    ctx->network.poll_fds[i].events = 0;
                    remove_poll_client((int)i, &ctx->network.poll_clients, &max_clients, &ctx->network.poll_fds);
                }
            }

            errno = 0;
        }
    }
}

struct pollfd *initialize_pollfds(int sockfd, int domain_fd, int **client_sockets)
{
    struct pollfd *fds;

    *client_sockets = NULL;

    fds = (struct pollfd *)malloc((REQUIRED_SOCKET_COUNT) * sizeof(struct pollfd));
    // printf("[initialize_pollfds DEBUG] Start\n");

    if(fds == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    fds[0].fd      = sockfd;
    fds[0].events  = POLLIN;
    fds[0].revents = 0;
    fds[1].fd      = domain_fd;
    fds[1].events  = POLLIN;
    fds[1].revents = 0;

    return fds;
}

void handle_new_connection(int sockfd, int **client_sockets, nfds_t *max_clients, struct pollfd **fds)
{
    // printf("[handle_new_connection DEBUG] Start\n");

    if((*fds)[0].revents & POLLIN && sockfd == (*fds)[0].fd)
    {
        socklen_t               addrlen;
        int                     new_socket;
        int                    *temp;
        struct sockaddr_storage addr;

        addrlen    = sizeof(addr);
        new_socket = accept((*fds)[0].fd, (struct sockaddr *)&addr, &addrlen);
        // fprintf(stderr, "[handle_new_connection DEBUG] accepted new socket: %d\n", new_socket);
        if(*max_clients == 1)
        {
            fprintf(stderr, "[DEBUG] First client, sleeping 1s to observe\n");
            sleep(1);
        }

        if(new_socket == -1)
        {
            perror("Accept error");
            exit(EXIT_FAILURE);
        }

        (*max_clients)++;
        temp = (int *)realloc(*client_sockets, sizeof(int) * (*max_clients));

        if(temp == NULL)
        {
            perror("realloc");
            free(*client_sockets);
            exit(EXIT_FAILURE);
        }
        else
        {
            struct pollfd *new_fds;
            *client_sockets                       = temp;
            (*client_sockets)[(*max_clients) - 1] = new_socket;

            new_fds = (struct pollfd *)realloc(*fds, ((*max_clients) + REQUIRED_SOCKET_COUNT) * sizeof(struct pollfd));
            if(new_fds == NULL)
            {
                perror("realloc");
                free(*client_sockets);
                exit(EXIT_FAILURE);
            }
            else
            {
                struct pollfd new_fd;
                *fds = new_fds;

                new_fd.fd = new_socket;
#ifdef __linux
                new_fd.events = POLLIN | POLLRDHUP | POLLHUP;
#endif
#ifdef __APPLE__
                new_fd.events = POLLIN | POLL_HUP | POLLHUP;
#endif
                new_fd.revents = 0;

                (*fds)[*max_clients + REQUIRED_SOCKET_COUNT - 1] = new_fd;
            }
        }
    }
}

void remove_poll_client(int poll_index, int **client_sockets, nfds_t *max_clients, struct pollfd **fds)
{
    int client_index = poll_index - REQUIRED_SOCKET_COUNT;
    // printf("[remove_poll_client DEBUG] Start\n");
    // printf("[remove_poll_client DEBUG] Closing socket: %d\n", (*fds)[poll_index].fd);

    if((nfds_t)client_index >= *max_clients || poll_index < REQUIRED_SOCKET_COUNT)
    {
        perror("Poll index out of range");
        return;
    }

    close((*fds)[poll_index].fd);
    (*fds)[poll_index].fd           = -1;
    (*fds)[poll_index].events       = 0;
    (*fds)[poll_index].revents      = 0;
    (*client_sockets)[client_index] = -1;

    (*max_clients)--;
    if(*max_clients == 0)
    {
        free(*client_sockets);
        *client_sockets = NULL;
        return;
    }

    if((nfds_t)client_index == (*max_clients))
    {    // Handle if the client index is the last one, realloc with -1 size
        int *temp;
        temp = (int *)realloc(*client_sockets, sizeof(int) * (*max_clients));

        if(temp == NULL)
        {
            perror("realloc");
            free(*client_sockets);
            exit(EXIT_FAILURE);
        }
        else
        {
            struct pollfd *new_fds;
            *client_sockets = temp;

            new_fds = (struct pollfd *)realloc(*fds, ((*max_clients) + REQUIRED_SOCKET_COUNT) * sizeof(struct pollfd));
            if(new_fds == NULL)
            {
                perror("realloc");
                free(*client_sockets);
                exit(EXIT_FAILURE);
            }
            *fds = new_fds;
        }
    }
    else
    {    // Handle if the client index is not the last one, replace it with the last one and realloc

        int           swap;
        struct pollfd poll_swap;
        int          *temp;

        // Swap client_sockets
        swap                            = (*client_sockets)[*max_clients];
        (*client_sockets)[*max_clients] = (*client_sockets)[client_index];
        (*client_sockets)[client_index] = swap;

        // Swap pollfds
        poll_swap                                    = (*fds)[*max_clients + REQUIRED_SOCKET_COUNT];
        (*fds)[*max_clients + REQUIRED_SOCKET_COUNT] = (*fds)[poll_index];
        (*fds)[poll_index]                           = poll_swap;

        temp = (int *)realloc(*client_sockets, sizeof(int) * (*max_clients));

        if(temp == NULL)
        {
            perror("realloc");
            free(*client_sockets);
            exit(EXIT_FAILURE);
        }
        else
        {
            struct pollfd *new_fds;
            *client_sockets = temp;

            new_fds = (struct pollfd *)realloc(*fds, ((*max_clients) + REQUIRED_SOCKET_COUNT) * sizeof(struct pollfd));
            if(new_fds == NULL)
            {
                perror("realloc");
                free(*client_sockets);
                exit(EXIT_FAILURE);
            }
            *fds = new_fds;
        }
    }
}

int recv_fd(int socket)
{
    struct msghdr   msg = {0};
    struct iovec    io;
    char            buf[1];
    struct cmsghdr *cmsg;
    char            control[CMSG_SPACE(sizeof(int))];
    int             fd;
    io.iov_base        = buf;
    io.iov_len         = sizeof(buf);
    msg.msg_iov        = &io;
    msg.msg_iovlen     = 1;
    msg.msg_control    = control;
    msg.msg_controllen = sizeof(control);
    // printf("[recv_fd DEBUG] Start\n");

    if(recvmsg(socket, &msg, 0) < 0)
    {
        perror("recvmsg");
        exit(EXIT_FAILURE);
    }
    cmsg = CMSG_FIRSTHDR(&msg);
    if(cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
    {
        memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
        return fd;
    }
    return -1;
}
