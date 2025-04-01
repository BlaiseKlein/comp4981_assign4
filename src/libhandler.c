
//
// Created by kari on 1/22/25.
//
//gcc -shared -fPIC -o libhandler.so src/libhandler.c -Iinclude -lgdbm_compat -lgdbm
#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "serve_request.h"
#include <ctype.h>
#include <errno.h>
#include <ndbm.h>
#include <time.h>
#define BUFFER_SIZE 1024
#define SERVER_ROOT "."

#define OK 200
#define NOT_FOUND 404
#define BAD_REQUEST 400
#define FORBIDDEN 403
#define SERVER_ERROR 500

#define BASE 16
void handle_get_request(int client_fd, char *resource_path);
void handle_head_request(int client_fd, char *resource_path);
void handle_post_request(int client_fd, struct thread_state *state);
void *http_respond(struct thread_state *ts)
{
        if (!ts || !ts->resource_string)
    {
        const char *resp = "HTTP/1.0 400 Bad Request\r\n\r\nMissing or malformed request.\n";
        write(ts->client_fd, resp, strlen(resp));
        return NULL;
    }
    const char *msg;
    printf("[LIBRARY] Handling request: %s\n", ts->resource_string);
if (ts->method == GET)
    handle_get_request(ts->client_fd, ts->resource_string);
else if (ts->method == HEAD)
    handle_head_request(ts->client_fd, ts->resource_string);
else if (ts->method == POST)
    handle_post_request(ts->client_fd, ts);
else {
    const char *resp = "HTTP/1.0 405 Method Not Allowed\r\n\r\n";
    write(ts->client_fd, resp, strlen(resp));
}
    return NULL;
}
//void *http_respond(struct thread_state *state)
//{
//
//    char *resource = state->resource_string;
//        printf("[LIBRARY] Handling request: %s\n", state->resource_string);
//
//    if (!resource || strlen(resource) == 0)
//    {
//        write(state->client_fd, "HTTP/1.0 400 Bad Request\r\n\r\n", 28);
//        return NULL;
//    }
//
//    // Check method
//    if (state->method == GET)
//    {
//        handle_get_request(state->client_fd, resource);
//    }
//    else if (state->method == HEAD)
//    {
//        handle_head_request(state->client_fd, resource);
//    }
//    else if (state->method == POST)
//    {
//        handle_post_request(state->client_fd, state);
//    }
//    else
//    {
//        write(state->client_fd, "HTTP/1.0 405 Method Not Allowed\r\n\r\n", 36);
//    }
//
//    return NULL;
//}


const char *get_mime_type(const char *filepath)
{
    // printf("inside mime_type %s\n", filepath);
    if(strstr(filepath, ".html"))
    {
        return "text/html";
    }
    if(strstr(filepath, ".css"))
    {
        return "text/css";
    }
    if(strstr(filepath, ".js"))
    {
        return "application/javascript";
    }
    if(strstr(filepath, ".jpeg") || strstr(filepath, ".jpg"))
    {
        return "image/jpeg";
    }
    if(strstr(filepath, ".png"))
    {
        return "image/png";
    }
    if(strstr(filepath, ".gif"))
    {
        return "image/gif";
    }
    if(strstr(filepath, ".swf"))
    {
        return "application/x-shockwave-flash";
    }
    return "application/octet-stream";
}

void handle_get_request(int client_fd, char *resource_path)
{
    char        file_path[BUFFER_SIZE];
    char        header[BUFFER_SIZE];
    char        file_buffer[BUFFER_SIZE];
    struct stat path_stat;
    int         filefd;
    ssize_t     bytes_read;
    const char *mime_type;

    int status_code = construct_and_validate_path(resource_path, file_path, sizeof(file_path), &path_stat);
    // printf("inside handle get request %s\n", file_path);
    if(status_code != OK)
    {
        // printf("%d\n", status_code);
        construct_http_header(header, sizeof(header), status_code, "text/plain", 0);
        write(client_fd, header, strlen(header));
        return;
    }

    mime_type = get_mime_type(file_path);

    // Open file for reading
    filefd = open(file_path, O_RDONLY | O_CLOEXEC);
    if(filefd < 0)
    {
        construct_http_header(header, sizeof(header), SERVER_ERROR, "text/plain", 0);
        write(client_fd, header, strlen(header));
        return;
    }

    construct_http_header(header, sizeof(header), OK, mime_type, (size_t)path_stat.st_size);
    // printf("header: %s\n", header);
    write(client_fd, header, strlen(header));

    // Send file content
    while((bytes_read = read(filefd, file_buffer, sizeof(file_buffer))) > 0)
    {
        write(client_fd, file_buffer, (size_t)bytes_read);
    }

    close(filefd);
}

void handle_head_request(int client_fd, char *resource_path)
{
    char        file_path[BUFFER_SIZE];
    char        header[BUFFER_SIZE];
    struct stat path_stat;
    const char *mime_type;

    int status_code = construct_and_validate_path(resource_path, file_path, sizeof(file_path), &path_stat);
    if(status_code != OK)
    {
        construct_http_header(header, sizeof(header), status_code, "text/plain", 0);
        write(client_fd, header, strlen(header));
        return;
    }

    mime_type = get_mime_type(file_path);

    // Construct and send headers only
    construct_http_header(header, sizeof(header), OK, mime_type, (size_t)path_stat.st_size);
    write(client_fd, header, strlen(header));
}


void handle_post_request(int client_fd, struct thread_state *state)
{
    if (!state->content_length_header)
    {
        const char *resp = "HTTP/1.0 411 Length Required\r\n\r\n";
        write(client_fd, resp, strlen(resp));
        return;
    }

    size_t content_length = (size_t)atoi(state->content_length_header);
    if (content_length <= 0 || content_length > 4096)
    {
        const char *resp = "HTTP/1.0 400 Bad Request\r\n\r\n";
        write(client_fd, resp, strlen(resp));
        return;
    }

    // Read body
    char *body = malloc(content_length + 1);
    if (!body)
    {
        const char *resp = "HTTP/1.0 500 Internal Server Error\r\n\r\n";
        write(client_fd, resp, strlen(resp));
        return;
    }

    ssize_t read_bytes = read(client_fd, body, content_length);
    body[read_bytes] = '\0';

    // Open ndbm
    DBM *db = dbm_open("postdata", O_RDWR | O_CREAT, 0644);
    if (!db)
    {
        perror("dbm_open");
        const char *resp = "HTTP/1.0 500 Internal Server Error\r\n\r\n";
        write(client_fd, resp, strlen(resp));
        free(body);
        return;
    }

    // Use timestamp as key
    char key_buf[64];
    snprintf(key_buf, sizeof(key_buf), "%ld", time(NULL));
    datum key = { .dptr = key_buf, .dsize = (int)strlen(key_buf) };
    datum value = { .dptr = body, .dsize = (int)strlen(body) };

    if (dbm_store(db, key, value, DBM_INSERT) != 0)
    {
        const char *resp = "HTTP/1.0 500 DB Store Failed\r\n\r\n";
        write(client_fd, resp, strlen(resp));
    }
    else
    {
        const char *resp = "HTTP/1.0 200 OK\r\n\r\nData stored successfully.";
        write(client_fd, resp, strlen(resp));
    }

    dbm_close(db);
    free(body);
}

int construct_and_validate_path(char *resource_path, char *file_path, size_t file_path_size, struct stat *path_stat)
{
    parse_url_encoding(resource_path);

    // Construct the file path
    snprintf(file_path, file_path_size, "%s%s", SERVER_ROOT, resource_path);
    // printf("construct and validate %s\n", file_path);
    // Default to index.html for directory-like paths
    if(resource_path[strlen(resource_path) - 1] == '/')
    {
        strncat(file_path, "index.html", file_path_size - strlen(file_path) - 1);
    }

    // Prevent directory traversal attacks
    if(strstr(resource_path, "/../"))
    {
        printf("directory traversal\n");
        return FORBIDDEN;
    }

    // Check file existence
    if(stat(file_path, path_stat) < 0)
    {
        if(errno == EACCES)
        {
            // printf("Permission denied for %s\n", file_path);
            return FORBIDDEN;
        }

        {
            // printf("File not found: %s\n", file_path);
            return NOT_FOUND;
        }
    }

    return OK;
}

void construct_http_header(char *header, size_t header_size, int status_code, const char *mime_type, size_t content_length)
{
    const char *status_text;

    switch(status_code)
    {
        case OK:
            status_text = "200 OK";
            break;
        case BAD_REQUEST:
            status_text = "400 Bad Request";
            break;
        case FORBIDDEN:
            status_text = "403 Forbidden";
            break;
        case NOT_FOUND:
            status_text = "404 Not Found";
            break;
        default:
            status_text = "500 Internal Server Error";
            break;
    }

    snprintf(header,
             header_size,
             "HTTP/1.0 %s\r\n"
             "Server: webserver-c\r\n"
             "Content-Length: %zu\r\n"
             "Content-Type: %s\r\n\r\n",
             status_text,
             content_length,
             mime_type);
}

char hex_char_to_char(const char *c)
{
    const char temp[3] = {c[0], c[1], '\0'};    // Copy two characters and null-terminate
    long       num     = strtol(temp, NULL, BASE);
    return (char)num;
}

void parse_url_encoding(char *resource_string)
{
    size_t resource_string_size = strlen(resource_string);
    char  *buffer               = (char *)malloc(resource_string_size + 1);
    char  *start                = buffer;

    for(size_t i = 0; i < resource_string_size; i++, buffer++)
    {
        if(resource_string[i] == '%' && i + 2 < resource_string_size)
        {
            *buffer = hex_char_to_char(&resource_string[i + 1]);
            i += 2;    // Skip the two hex characters
        }
        else
        {
            *buffer = resource_string[i];
        }
    }
    *buffer = '\0';    // Null-terminate the result

    // Copy decoded result back to resource_string
    strncpy(resource_string, start, resource_string_size);
    resource_string[resource_string_size] = '\0';

    free(start);
}
