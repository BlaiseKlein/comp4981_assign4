//
// Created by blaise-klein on 2/12/25.
//

#ifndef DATA_TYPES_H
#define DATA_TYPES_H
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/types.h>

#define DOMAINSOCKET "/tmp/domainsock"
#define CHILD_COUNT 3

struct network_state
{
    /* cppcheck-suppress unusedStructMember */
    int receive_fd;
    /* cppcheck-suppress unusedStructMember */
    struct sockaddr_storage *receive_addr;
    /* cppcheck-suppress unusedStructMember */
    socklen_t receive_addr_len;
    /* cppcheck-suppress unusedStructMember */
    in_port_t receive_port;
    /* cppcheck-suppress unusedStructMember */
    int next_client_fd;
    /* cppcheck-suppress unusedStructMember */
    int domain_fd[2];
    /* cppcheck-suppress unusedStructMember */
    pid_t child_pids[CHILD_COUNT];
    /* cppcheck-suppress unusedStructMember */
    struct pollfd *poll_fds;
    /* cppcheck-suppress unusedStructMember */
    int *poll_clients;
};

struct arguments
{
    /* cppcheck-suppress unusedStructMember */
    char *sys_addr;
    /* cppcheck-suppress unusedStructMember */
    ssize_t sys_addr_len;
    /* cppcheck-suppress unusedStructMember */
    char *sys_port;
};

struct library_info
{
    /* cppcheck-suppress unusedStructMember */
    void *handle;    // library handle from dlopen()
    /* cppcheck-suppress unusedStructMember */
    time_t last_mod;
    /* cppcheck-suppress unusedStructMember */
    const char *path;    // path to library
};

struct context
{
    /* cppcheck-suppress unusedStructMember */
    struct arguments arg;
    /* cppcheck-suppress unusedStructMember */
    struct network_state network;
    /* cppcheck-suppress unusedStructMember */
    int err;
    /* cppcheck-suppress unusedStructMember */
    pid_t watcher_thread;
    /* cppcheck-suppress unusedStructMember */
    struct library_info lib_info;
};

#endif    // DATA_TYPES_H
