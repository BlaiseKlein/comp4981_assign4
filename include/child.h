//
// Created by kari on 3/18/25.
//

#ifndef CHILD_H
#define CHILD_H

#include "data_types.h"
#include "http.h"
#include <dlfcn.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

int  await_request(void);
void check_library(struct context *ctx);
int  load_library(struct context *ctx, const char *path);
int  return_fd(void);
int  send_response(void);
int  handle_request(struct context *ctx, int client_fd);

#endif    // CHILD_H
