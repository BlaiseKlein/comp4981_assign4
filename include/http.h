//
// Created by blaise-klein on 1/9/25.
//

#ifndef HTTP_H
#define HTTP_H

#include "http.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <ndbm.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define SERVER_ROOT "."
#define MAXMESSAGELENGTH 4096
#define MAXLINELENGTH 2048
#define OK 200
#define NOT_FOUND 404
#define BAD_REQUEST 400
#define FORBIDDEN 403
#define SERVER_ERROR 500

#define BASE 16

enum http_method
{
    GET,
    HEAD,
    POST,
    UNSUPPORTED
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

void       *parse_request(void *data);
size_t      read_until(int fd, char *buffer, size_t len, const char *delimiter, int *err);
int         parse_request_headers(struct thread_state *data);
int         parse_request_line(struct thread_state *data);
void        cleanup_headers(struct thread_state *data);
void        cleanup_header(char *header);
int         parse_header(struct thread_state *data, char **buffer, bool *breaks, bool *continues);
void        parse_path_arguments(const char *start_resource_string, char *end_resource_string);
void       *http_respond(struct thread_state *data);
void        handle_get_request(int client_fd, char *resource_path);
void        handle_head_request(int client_fd, char *resource_path);
void        handle_post_request(int client_fd, struct thread_state *state);
int         construct_and_validate_path(char *resource_path, char *file_path, size_t file_path_size, struct stat *path_stat);
char        hex_char_to_char(const char *c);
void        construct_http_header(char *header, size_t header_size, int status_code, const char *mime_type, size_t content_length);
const char *get_mime_type(const char *filepath);
void        parse_url_encoding(char *resource_string);

#endif    // HTTP_H
