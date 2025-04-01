//
// Created by blaise-klein on 1/9/25.
//

#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void *parse_request(void *context_data)
{
    char                 divider[MAXLINELENGTH];
    int                  header_result       = 0;
    int                  request_line_result = 0;
    int                  err                 = 0;
    ssize_t              result              = 0;
    struct thread_state *data                = (struct thread_state *)context_data;
    data->request_line_string                = (char *)malloc((MAXLINELENGTH) * sizeof(char));
    if(data->request_line_string == NULL)
    {
        return 0;
    }

    result = (ssize_t)read_until(data->client_fd, data->request_line_string, MAXLINELENGTH * sizeof(char), "\r\n", &err);
    if(result < 0)
    {
        free(data->request_line_string);
        return NULL;
    }

    data->request_line_string[result] = '\0';
    data->request_line_string_len     = strnlen(data->request_line_string, MAXLINELENGTH);
    request_line_result               = parse_request_line(data);
    if(request_line_result < 0)
    {
        free(data->request_line_string);
        return NULL;
    }
    result = (ssize_t)read_until(data->client_fd, divider, MAXLINELENGTH * sizeof(char), "\r\n", &err);
    if(result < 0)
    {
        free(data->request_line_string);
        return NULL;
    }

    header_result = parse_request_headers(data);
    if(header_result < 0)
    {
        free(data->request_line_string);
        return NULL;
    }

    return data;
}

// void *http_respond(struct thread_state *data)
// {
//     if(data->method == GET)
//     {
//         printf("GET request %s\n", data->resource_string);
//         handle_get_request(data->client_fd, data->resource_string);
//         return NULL;
//     }
//     if(data->method == HEAD)
//     {
//         printf("HEAD request\n");
//         handle_head_request(data->client_fd, data->resource_string);
//         return NULL;
//     }
//     if(data->method == POST)
//     {
//         printf("POST request %s\n", data->resource_string);
//         handle_post_request(state->client_fd, data);
//         return NULL;
//     }
//     printf("OTHER request\n");
//     {
//         const char *resp = "HTTP/1.0 405 Method Not Allowed\r\n"
//                            "Content-Type: text/plain\r\n\r\n"
//                            "405 Method Not Allowed";
//         write(data->client_fd, resp, strlen(resp));
//     }
//     close(data->client_fd);
//     return data;
// }

size_t read_until(int fd, char *buffer, size_t len, const char *delimiter, int *err)
{
    ssize_t buffer_end = 0;
    char   *message    = (char *)malloc(len);
    if(message == NULL)
    {
        *err = -1;
        return 0;
    }

    do
    {
        ssize_t result;

        result = read(fd, message + buffer_end, 1);
        if(result <= 0)
        {
            *err = -1;
            free(message);
            return 0;
        }

        if(buffer_end + result > (ssize_t)len)
        {
            *err = -2;
            free(message);
            return 0;
        }
        buffer_end += result;
    } while(buffer_end < 2 || (message[buffer_end - 2] != delimiter[0] && message[buffer_end - 1] != delimiter[1]));

    memset(buffer, 0, len);
    memcpy(buffer, message, (size_t)buffer_end);

    free(message);
    return (size_t)buffer_end;
}

void parse_path_arguments(const char *start_resource_string, char *end_resource_string)
{
    const char unix_slash      = '/';
    const char mark_start_args = '?';

    for(; start_resource_string != end_resource_string; end_resource_string--)
    {
        if(*end_resource_string == unix_slash)
        {
            return;
        }
        if(*end_resource_string == mark_start_args)
        {
            *end_resource_string = '\0';
            return;
        }
    }
}

int parse_request_line(struct thread_state *data)
{
    const char *method;
    const char *path;
    char       *version;
    const char *start_resource_string;
    char       *end_resource_string;
    char       *rest = data->request_line_string;

    method  = strtok_r(data->request_line_string, " ", &rest);
    path    = strtok_r(NULL, " ", &rest);
    version = strtok_r(NULL, " ", &rest);
    if(method == NULL)
    {
        return -1;
    }
    if(strcmp(method, "GET") == 0)
    {
        data->method = GET;
    }
    else if(strcmp(method, "HEAD") == 0)
    {
        data->method = HEAD;
    }
    else if(strcmp(method, "POST") == 0)
    {
        data->method = POST;
    }

    data->resource_string = (char *)malloc((MAXLINELENGTH) * sizeof(char));
    if(data->resource_string == NULL)
    {
        return -1;
    }
    end_resource_string = version;
    end_resource_string--;
    start_resource_string = path;
    parse_path_arguments(start_resource_string, end_resource_string);
    memcpy(data->resource_string, path, (size_t)(version - path + 1));

    data->version = (char *)malloc((MAXLINELENGTH) * sizeof(char));
    if(data->version == NULL)
    {
        free(data->resource_string);
        return -1;
    }
    memcpy(data->version, version, strlen(version) - 2);

    return 0;
}

int parse_header(struct thread_state *data, char **buffer, bool *breaks, bool *continues)
{
    char       *header;
    char       *info;
    const char *colon_place = NULL;
    info                    = (char *)malloc(MAXLINELENGTH);
    if(info == NULL)
    {
        return -1;
    }
    header = (char *)malloc(MAXLINELENGTH * sizeof(char));
    if(header == NULL)
    {
        free(info);
        return -1;
    }
    if((*buffer)[0] == '\r' && (*buffer)[1] == '\n')
    {
        free(info);
        free(header);
        *breaks = true;
        return -1;
    }

    // Get the position of the colon in the header string
    colon_place = strchr(*buffer, ':');
    if(colon_place == NULL)
    {
        free(header);
        free(info);
        return -1;
    }

    // Read up to colon_place and get the header name
    for(int i = 0; *buffer != colon_place; i++, (*buffer)++)
    {
        header[i] = (*buffer)[0];
    }

    // Read after the colon_place to get the header information
    (*buffer)++;    // One for the colon
    (*buffer)++;    // One more for the space
    memset(info, 0, MAXLINELENGTH);
    for(int i = 0; (*buffer)[0] != '\r' && (*buffer)[1] != '\n'; i++, (*buffer)++)
    {
        info[i] = (*buffer)[0];
    }

    // Switch statement to load the info into the correct place
    if(strcmp(header, "Date") == 0)
    {
        data->date_header = (char *)malloc(MAXLINELENGTH);
        if(data->date_header == NULL)
        {
            data->date_header = NULL;
            goto cleanup;
        }
        memcpy(data->date_header, info, MAXLINELENGTH);
    }
    else if(strcmp(header, "Pragma") == 0)
    {
        data->pragma_header = (char *)malloc(MAXLINELENGTH);
        if(data->pragma_header == NULL)
        {
            data->pragma_header = NULL;
            goto cleanup;
        }
        memcpy(data->pragma_header, info, MAXLINELENGTH);
    }
    else if(strcmp(header, "Author") == 0)
    {
        data->auth_header = (char *)malloc(MAXLINELENGTH);
        if(data->auth_header == NULL)
        {
            data->auth_header = NULL;
            goto cleanup;
        }
        memcpy(data->auth_header, info, MAXLINELENGTH);
    }
    else if(strcmp(header, "From") == 0)
    {
        data->from_header = (char *)malloc(MAXLINELENGTH);
        if(data->from_header == NULL)
        {
            data->from_header = NULL;
            goto cleanup;
        }
        memcpy(data->from_header, info, MAXLINELENGTH);
    }
    else if(strcmp(header, "If-Modified-Since") == 0)
    {
        data->if_modified_since_header = (char *)malloc(MAXLINELENGTH);
        if(data->if_modified_since_header == NULL)
        {
            data->if_modified_since_header = NULL;
            goto cleanup;
        }
        memcpy(data->if_modified_since_header, info, MAXLINELENGTH);
    }
    else if(strcmp(header, "Referer") == 0)
    {
        data->referer_header = (char *)malloc(MAXLINELENGTH);
        if(data->referer_header == NULL)
        {
            data->referer_header = NULL;
            goto cleanup;
        }
        memcpy(data->referer_header, info, MAXLINELENGTH);
    }
    else if(strcmp(header, "User-Agent") == 0)
    {
        data->user_agent_header = (char *)malloc(MAXLINELENGTH);
        if(data->user_agent_header == NULL)
        {
            data->user_agent_header = NULL;
            goto cleanup;
        }
        memcpy(data->user_agent_header, info, MAXLINELENGTH);
    }
    else if(strcmp(header, "Allow") == 0)
    {
        data->allow_header = (char *)malloc(MAXLINELENGTH);
        if(data->allow_header == NULL)
        {
            data->allow_header = NULL;
            goto cleanup;
        }
        memcpy(data->allow_header, info, MAXLINELENGTH);
    }
    else if(strcmp(header, "Content-Encoding") == 0)
    {
        data->content_encoding_header = (char *)malloc(MAXLINELENGTH);
        if(data->content_encoding_header == NULL)
        {
            data->content_encoding_header = NULL;
            goto cleanup;
        }
        memcpy(data->content_encoding_header, info, MAXLINELENGTH);
    }
    else if(strcmp(header, "Content-Length") == 0)
    {
        data->content_length_header = (char *)malloc(MAXLINELENGTH);
        if(data->content_length_header == NULL)
        {
            data->content_length_header = NULL;
            goto cleanup;
        }
        memcpy(data->content_length_header, info, MAXLINELENGTH);
    }
    else if(strcmp(header, "Content-Type") == 0)
    {
        data->content_type_header = (char *)malloc(MAXLINELENGTH);
        if(data->content_type_header == NULL)
        {
            data->content_type_header = NULL;
            goto cleanup;
        }
        memcpy(data->content_type_header, info, MAXLINELENGTH);
    }
    else if(strcmp(header, "Expires") == 0)
    {
        data->expires_header = (char *)malloc(MAXLINELENGTH);
        if(data->expires_header == NULL)
        {
            data->expires_header = NULL;
            goto cleanup;
        }
        memcpy(data->expires_header, info, MAXLINELENGTH);
    }
    else if(strcmp(header, "Last-Modified") == 0)
    {
        data->last_modified_header = (char *)malloc(MAXLINELENGTH);
        if(data->last_modified_header == NULL)
        {
            data->last_modified_header = NULL;
            goto cleanup;
        }
        memcpy(data->last_modified_header, info, MAXLINELENGTH);
    }

    free(info);
    free(header);
    *continues = true;
    return 0;

cleanup:
    free(info);
    free(header);
    cleanup_headers(data);
    return -1;
}

int parse_request_headers(struct thread_state *data)
{
    char *buffer = (char *)malloc(MAXLINELENGTH);
    char *start  = buffer;
    if(buffer == NULL)
    {
        return -1;
    }
    data->date_header              = NULL;
    data->pragma_header            = NULL;
    data->auth_header              = NULL;
    data->from_header              = NULL;
    data->if_modified_since_header = NULL;
    data->referer_header           = NULL;
    data->user_agent_header        = NULL;
    data->allow_header             = NULL;
    data->content_encoding_header  = NULL;
    data->content_length_header    = NULL;
    data->content_type_header      = NULL;
    data->expires_header           = NULL;
    data->last_modified_header     = NULL;

    while(read_until(data->client_fd, buffer, MAXLINELENGTH, "\r\n", &data->err) > 0)
    {
        bool breaks, continues;
        int  value;
        breaks    = false;
        continues = false;
        value     = parse_header(data, &buffer, &breaks, &continues);
        if(breaks)
        {
            break;
        }
        if(continues)
        {
            buffer = start;
            continue;
        }
        if(value < 0)
        {
            free(buffer);
            return value;
        }
    }

    free(start);
    return 0;
}

void cleanup_headers(struct thread_state *data)
{
    if(data != NULL)
    {
        cleanup_header(data->date_header);
        data->date_header = NULL;
        cleanup_header(data->pragma_header);
        data->pragma_header = NULL;
        cleanup_header(data->auth_header);
        data->auth_header = NULL;
        cleanup_header(data->from_header);
        data->from_header = NULL;
        cleanup_header(data->if_modified_since_header);
        data->if_modified_since_header = NULL;
        cleanup_header(data->referer_header);
        data->referer_header = NULL;
        cleanup_header(data->user_agent_header);
        data->user_agent_header = NULL;
        cleanup_header(data->allow_header);
        data->allow_header = NULL;
        cleanup_header(data->content_encoding_header);
        data->content_encoding_header = NULL;
        cleanup_header(data->content_length_header);
        data->content_length_header = NULL;
        cleanup_header(data->content_type_header);
        data->content_type_header = NULL;
        cleanup_header(data->expires_header);
        data->expires_header = NULL;
        cleanup_header(data->last_modified_header);
        data->last_modified_header = NULL;
    }
}

void cleanup_header(char *header)
{
    if(header != NULL)
    {
        free(header);
    }
}

//
// int handle_get_request(int fd, const char *resource)
// {
//     if(fd < 0 || resource == NULL)
//     {
//         return -1;
//     }
//     return 0;
// }
//
// int handle_head_request(int fd, const char *resource)
// {
//     if(fd < 0 || resource == NULL)
//     {
//         return -1;
//     }
//     return 0;
// }
