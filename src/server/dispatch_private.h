#ifndef GPUPROCESS_DISPATCH_PRIVATE_H
#define GPUPROCESS_DISPATCH_PRIVATE_H

#include "config.h"
#include "compiler_private.h"

typedef void (*FunctionPointerType)(void);

typedef struct _server server_t;
#include "dispatch_gles2_private.h"
#include "dispatch_private_desktop.h"

private void
dispatch_init (server_dispatch_table_t *dispatch);

#endif /* GPUPROCESS_DISPATCH_PRIVATE_H */
