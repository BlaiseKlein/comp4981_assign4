//
// Created by blaise-klein on 1/9/25.
//

#ifndef HTTP_H
#define HTTP_H

#define MAXMESSAGELENGTH 4096
#define MAXLINELENGTH 2048
#include <stdbool.h>
#include <stddef.h>

enum http_method
{
    GET,
    HEAD,
    POST
};

struct thread_state
{
    /* cppcheck-suppress unusedStructMember */
    int client_fd;
    /* cppcheck-suppress unusedStructMember */
    char *request_line_string;
    /* cppcheck-suppress unusedStructMember */
    size_t request_line_string_len;
    /* cppcheck-suppress unusedStructMember */
    char *date_header;
    /* cppcheck-suppress unusedStructMember */
    char *pragma_header;
    /* cppcheck-suppress unusedStructMember */
    char *auth_header;
    /* cppcheck-suppress unusedStructMember */
    char *from_header;
    /* cppcheck-suppress unusedStructMember */
    char *if_modified_since_header;
    /* cppcheck-suppress unusedStructMember */
    char *referer_header;
    /* cppcheck-suppress unusedStructMember */
    char *user_agent_header;
    /* cppcheck-suppress unusedStructMember */
    char *allow_header;
    /* cppcheck-suppress unusedStructMember */
    char *content_encoding_header;
    /* cppcheck-suppress unusedStructMember */
    char *content_length_header;
    /* cppcheck-suppress unusedStructMember */
    char *content_type_header;
    /* cppcheck-suppress unusedStructMember */
    char *expires_header;
    /* cppcheck-suppress unusedStructMember */
    char *last_modified_header;
    /* cppcheck-suppress unusedStructMember */
    char *resource_string;
    /* cppcheck-suppress unusedStructMember */
    enum http_method method;
    /* cppcheck-suppress unusedStructMember */
    int err;
    /* cppcheck-suppress unusedStructMember */
    char *version;
};

void  *parse_request(void *data);
size_t read_until(int fd, char *buffer, size_t len, const char *delimiter, int *err);
int    parse_request_headers(struct thread_state *data);
int    parse_request_line(struct thread_state *data);
void   cleanup_headers(struct thread_state *data);
void   cleanup_header(char *header);
int    parse_header(struct thread_state *data, char **buffer, bool *breaks, bool *continues);
void   parse_path_arguments(const char *start_resource_string, char *end_resource_string);
void  *http_respond(struct thread_state *data);
// int    handle_get_request(int fd, const char *resource);
// int    handle_head_request(int fd, const char *resource);

#endif    // HTTP_H
