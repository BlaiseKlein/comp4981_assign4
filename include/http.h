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
    HEAD
};

struct thread_state
{
    int              client_fd;
    char            *request_line_string;
    size_t           request_line_string_len;
    char            *date_header;
    char            *pragma_header;
    char            *auth_header;
    char            *from_header;
    char            *if_modified_since_header;
    char            *referer_header;
    char            *user_agent_header;
    char            *allow_header;
    char            *content_encoding_header;
    char            *content_length_header;
    char            *content_type_header;
    char            *expires_header;
    char            *last_modified_header;
    char            *resource_string;
    enum http_method method;
    int              err;
    char            *version;
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
int    handle_get_request(int fd, const char *resource);
int    handle_head_request(int fd, const char *resource);

#endif    // HTTP_H
