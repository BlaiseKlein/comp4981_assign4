//
// Created by blaise-klein on 3/6/25.
//

#ifndef REQ_HANDLER_H
#define REQ_HANDLER_H

#include "data_types.h"

int  setup_children(struct context *ctx);
void startup_child(struct context *ctx);

#endif    // REQ_HANDLER_H
