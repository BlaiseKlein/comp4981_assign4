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

void check_library(struct context *ctx);
int  handle_request(struct context *ctx, int client_fd);

#endif    // CHILD_H
