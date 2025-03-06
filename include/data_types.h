//
// Created by blaise-klein on 2/12/25.
//

#ifndef DATA_TYPES_H
#define DATA_TYPES_H
#include <netinet/in.h>
#include <sys/types.h>

#define DOMAINSOCKET "/tmp/domainsock"
#define CHILD_COUNT 3

struct network_state
{
    int                      receive_fd;
    struct sockaddr_storage *receive_addr;
    socklen_t                receive_addr_len;
    in_port_t                receive_port;
    int                      next_client_fd;
    int                      domain_fd;
    pid_t                    child_pids[CHILD_COUNT];
};

struct arguments
{
    char   *sys_addr;
    ssize_t sys_addr_len;
    char   *sys_port;
};

struct context
{
    struct arguments     arg;
    struct network_state network;
    int                  err;
    char               **paths;
    int                  paths_len;
};

#endif    // DATA_TYPES_H
