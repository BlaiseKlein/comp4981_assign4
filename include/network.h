//
// Created by blaise-klein on 2/12/25.
//

#ifndef NETWORK_H
#define NETWORK_H

#include "data_types.h"
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/poll.h>
#include <unistd.h>

in_port_t      parse_in_port_t(const char *port_str, int *err);
void           convert_address(const char *address, struct sockaddr_storage *addr, socklen_t *addr_len, int *err);
int            socket_create(int domain, int type, int protocol, int *err);
void           socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port, int *err);
void           socket_close(int sockfd);
void           get_address_to_server(struct sockaddr_storage *addr, in_port_t port, int *err);
ssize_t        read_fully(int sockfd, void *buf, ssize_t len, int *err);
ssize_t        write_fully(int sockfd, const void *buf, ssize_t len, int *err);
int            setup_socket(struct context *ctx);
int            client_connect(struct context *ctx);
int            setup_domain_socket(struct context *ctx);
_Noreturn int  await_client_connection(struct context *ctx);
int            forward_request(const struct context *ctx);
struct pollfd *initialize_pollfds(int sockfd, int domain_fd, int **client_sockets);
void           handle_new_connection(int sockfd, int **client_sockets, nfds_t *max_clients, struct pollfd **fds);
void           remove_poll_client(int poll_index, int **client_sockets, nfds_t *max_clients, struct pollfd **fds);
int            recv_fd(int socket);
void           send_fd(int socket, int fd);

static int connect_to_server(struct sockaddr_storage *addr, socklen_t addr_len, int *err)
{
    int fd;
    int result;

    fd = socket(addr->ss_family, SOCK_STREAM, 0);    // NOLINT(android-cloexec-socket)

    if(fd == -1)
    {
        *err = errno;
        goto done;
    }

    result = connect(fd, (const struct sockaddr *)addr, addr_len);

    if(result == -1)
    {
        *err = errno;
        close(fd);
        fd = -1;
    }

done:
    return fd;
}

#endif    // NETWORK_H
