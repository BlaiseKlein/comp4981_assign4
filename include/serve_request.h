//
// Created by kari on 1/22/25.
//

#ifndef SERVE_REQUEST_H
#define SERVE_REQUEST_H
#include <stddef.h>
#include <sys/stat.h>
const char *get_mime_type(const char *filepath);
//void        handle_get_request(int client_fd, char *resource_path);
//void        handle_head_request(int client_fd, char *resource_path);
int         construct_and_validate_path(char *resource_path, char *file_path, size_t file_path_size, struct stat *path_stat);
void        construct_http_header(char *header, size_t header_size, int status_code, const char *mime_type, size_t content_length);
char        hex_char_to_char(const char *c);
void        parse_url_encoding(char *resource_string);
#endif    // SERVE_REQUEST_H
