//
// Created by blaise-klein on 1/9/25.
//
//gcc -shared -fPIC -o http.so src/http.c -Iinclude -lgdbm_compat -lgdbm
#include "http.h"

void *parse_request(void *context_data)
{
    char                 divider[MAXLINELENGTH];
    int                  header_result       = 0;
    int                  request_line_result = 0;
    int                  err                 = 0;
    ssize_t              result              = 0;
    struct thread_state *data                = (struct thread_state *)context_data;
    data->request_line_string                = (char *)malloc((MAXLINELENGTH) * sizeof(char));
    printf("HI FROM LIBRARY\n");

    if(data->request_line_string == NULL)
    {
        return 0;
    }

    result = (ssize_t)read_until(data->client_fd, data->request_line_string, MAXLINELENGTH * sizeof(char), "\r\n", &err);
    if(result < 0)
    {
        fprintf(stderr, "[parse_request DEBUG] Failed to read request line\n");
        free(data->request_line_string);
        return NULL;
    }

    data->request_line_string[result] = '\0';
//    fprintf(stderr, "[parse_request DEBUG] Raw request line: '%s'\n", data->request_line_string);
    // Gracefully ignore empty connections
    if(strlen(data->request_line_string) == 0)
    {
        // No request came in — maybe speculative or keep-alive socket
        return NULL;
    }
    data->request_line_string_len = strnlen(data->request_line_string, MAXLINELENGTH);
    request_line_result           = parse_request_line(data);
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

size_t read_until(int fd, char *buffer, size_t len, const char *delimiter, int *err)
{
    ssize_t buffer_end = 0;
    char   *message    = (char *)malloc(len);
//    printf("[read_until DEBUG] Start\n");

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
//    printf("[parse_path_arguments DEBUG] Start\n");

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
//    printf("[parse_request_line DEBUG] Start\n");

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
    } else
    {
        data->method = UNSUPPORTED;
    }

    data->resource_string = (char *)malloc((MAXLINELENGTH) * sizeof(char));
    if(data->resource_string == NULL)
    {
        return -1;
    }
    end_resource_string = version;
    end_resource_string--;
    start_resource_string = path;
    if (path == NULL || version == NULL) {
        return -1;
    }

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
//    printf("[parse_header DEBUG] Start\n");

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
        header[i]     = (*buffer)[0];
        header[i + 1] = '\0';    // ← Add this line (or do it after the loop)
    }

    // Read after the colon_place to get the header information
    (*buffer)++;    // One for the colon
    while(**buffer == ' ')
    {
        (*buffer)++;
    }
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
//    printf("[parse_request_headers DEBUG] Start\n");

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
//    printf("[cleanup_headers DEBUG] Start\n");

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
//    printf("[cleanup_header DEBUG] Start\n");

    if(header != NULL)
    {
        free(header);
    }
}

void *http_respond(struct thread_state *ts)
{
//    printf("[http_respond DEBUG] Start\n");

    if(!ts || !ts->resource_string)
    {
        const char *resp = "HTTP/1.0 400 Bad Request\r\n\r\nMissing or malformed request\n";
        write(ts->client_fd, resp, strlen(resp));
        return NULL;
    }
    const char *msg;
    // printf("[DEBUG] content_length_header: %s\n", ts->content_length_header);

//    printf("[http_request DEBUG] Handling request: %s (method %d)\n", ts->resource_string, ts->method);

    if(ts->method == GET)
    {
        handle_get_request(ts->client_fd, ts->resource_string);
    }
    else if(ts->method == HEAD)
    {
        handle_head_request(ts->client_fd, ts->resource_string);
    }
    else if(ts->method == POST)
    {
        handle_post_request(ts->client_fd, ts);
    }
    else
    {
        const char *resp = "HTTP/1.0 405 Method Not Allowed\r\n\r\n";
        write(ts->client_fd, resp, strlen(resp));
        shutdown(ts->client_fd, SHUT_WR);

    }
    return NULL;
}

const char *get_mime_type(const char *filepath)
{
//    printf("[get_mime_type DEBUG] Start\n");

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
//    printf("[handle_get_request DEBUG] Start\n");

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

        shutdown(client_fd, SHUT_WR);
}

void handle_head_request(int client_fd, char *resource_path)
{
    char        file_path[BUFFER_SIZE];
    char        header[BUFFER_SIZE];
    struct stat path_stat;
    const char *mime_type;

    int status_code = construct_and_validate_path(resource_path, file_path, sizeof(file_path), &path_stat);
//    printf("[handle_head_request DEBUG] Start\n");

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
//    printf("[handle_post_request DEBUG] Start\n");

    if(!state->content_length_header)
    {
        const char *resp = "HTTP/1.0 411 Length Required\r\n\r\n";
        write(client_fd, resp, strlen(resp));
        //        shutdown(client_fd, SHUT_WR);
        return;
    }

    size_t content_length = (size_t)atoi(state->content_length_header);
    if(content_length <= 0 || content_length > 4096)
    {
        const char *resp = "HTTP/1.0 400 Bad Request\r\n\r\n";
        write(client_fd, resp, strlen(resp));
        //  printf("[DEBUG] Invalid Content-Length: %zu\n", content_length);
        //        shutdown(client_fd, SHUT_WR);
        return;
    }

    char *body = malloc(content_length);
    if(!body)
    {
        const char *resp = "HTTP/1.0 500 Internal Server Error\r\n\r\n";
        write(client_fd, resp, strlen(resp));
        perror("[DEBUG] malloc failed for body");
        //        shutdown(client_fd, SHUT_WR);
        return;
    }

    ssize_t total_read = 0;
    while(total_read < (ssize_t)content_length)
    {
        ssize_t n = read(client_fd, body + total_read, content_length - total_read);
        if(n <= 0)
        {
            perror("[DEBUG] Failed to read full POST body");
            const char *resp = "HTTP/1.0 400 Bad Request\r\n\r\nIncomplete POST body.\n";
            write(client_fd, resp, strlen(resp));
            free(body);
            //            shutdown(client_fd, SHUT_WR);
            return;
        }
        total_read += n;
    }

    // printf("[DEBUG] Received body (%zd bytes): '%.*s'\n", total_read, (int)total_read, body);

    DBM *db = dbm_open("postdata", O_RDWR | O_CREAT, 0644);
    if(!db)
    {
        perror("dbm_open");
        const char *resp = "HTTP/1.0 500 Internal Server Error\r\n\r\n";
        write(client_fd, resp, strlen(resp));
        free(body);
        //        shutdown(client_fd, SHUT_WR);
        return;
    }

    // Create a timestamp key (heap allocated)
    char timestamp_buf[64];
    snprintf(timestamp_buf, sizeof(timestamp_buf), "%ld", time(NULL));

    datum key;
    key.dsize = (int)(strlen(timestamp_buf) + 1);    // include null terminator
    key.dptr  = malloc(key.dsize);
    if(!key.dptr)
    {
        const char *resp = "HTTP/1.0 500 Internal Server Error\r\n\r\n";
        write(client_fd, resp, strlen(resp));
        perror("[DEBUG] malloc failed for key.dptr");
        dbm_close(db);
        free(body);
        //        shutdown(client_fd, SHUT_WR);
        return;
    }
    memcpy(key.dptr, timestamp_buf, key.dsize);

    datum value;
    value.dsize = (int)total_read;
    value.dptr  = malloc(value.dsize);
    if(!value.dptr)
    {
        const char *resp = "HTTP/1.0 500 Internal Server Error\r\n\r\n";
        write(client_fd, resp, strlen(resp));
        perror("[DEBUG] malloc failed for value.dptr");
        dbm_close(db);
        free(body);
        free(key.dptr);
        //        shutdown(client_fd, SHUT_WR);
        return;
    }

    memcpy(value.dptr, body, value.dsize);
    free(body);

    //    printf("[DEBUG] key='%s', key.dsize=%d | value.dsize=%d\n", key.dptr, key.dsize, value.dsize);

    if(key.dsize == 0 || value.dsize == 0)
    {
        const char *resp = "HTTP/1.0 400 Bad Request\r\n\r\nEmpty key or value not allowed.\n";
        write(client_fd, resp, strlen(resp));
        //  fprintf(stderr, "[DEBUG] Refusing to store empty key or value\n");
        dbm_close(db);
        free(value.dptr);
        free(key.dptr);
        //        shutdown(client_fd, SHUT_WR);
        return;
    }

    // printf("[DEBUG] dbm_store key='%.*s' (%d), value='%.*s' (%d)\n",
    //           key.dsize, key.dptr, key.dsize,
    //           value.dsize, value.dptr, value.dsize);

    if(dbm_store(db, key, value, DBM_REPLACE) != 0)
    {
        const char *resp = "HTTP/1.0 500 DB Store Failed\r\n\r\n";
        write(client_fd, resp, strlen(resp));
        perror("[DEBUG] dbm_store failed");
    }
    else
    {
        char header[BUFFER_SIZE];
        construct_http_header(header, sizeof(header), OK, "text/plain", strlen("Data stored successfully.\n"));
        write(client_fd, header, strlen(header));
        write(client_fd, "Data stored successfully.\n", strlen("Data stored successfully.\n"));
        // shutdown(client_fd, SHUT_WR);

        // printf("[POST] Stored key: '%s' (%d) | value: '%.*s' (%d)\n",
        //               key.dptr, key.dsize, value.dsize, value.dptr, value.dsize);
    }

    dbm_close(db);
    free(value.dptr);
    free(key.dptr);
    //    shutdown(client_fd, SHUT_WR);
}

int construct_and_validate_path(char *resource_path, char *file_path, size_t file_path_size, struct stat *path_stat)
{
//    printf("[construct_and_validate_path DEBUG] Start\n");

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
//        printf("directory traversal\n");
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
//    printf("[construct_http_header DEBUG] Start\n");

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
//    printf("[hex_char_to_char DEBUG] Start\n");

    return (char)num;
}

void parse_url_encoding(char *resource_string)
{
    size_t resource_string_size = strlen(resource_string);
    char  *buffer               = (char *)malloc(resource_string_size + 1);
    char  *start                = buffer;
//    printf("[parse_url_encoding DEBUG] Start\n");

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
