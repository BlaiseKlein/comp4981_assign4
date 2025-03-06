//
// Created by blaise-klein on 3/4/25.
//

#ifndef SERVER_H
#define SERVER_H
#include "data_types.h"

void           parse_arguments(int argc, char *argv[], struct context *ctx);
_Noreturn void usage(const char *program_name, int exit_code, const char *message);

#endif    // SERVER_H
