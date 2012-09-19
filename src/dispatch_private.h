#ifndef GPUPROCESS_DISPATCH_PRIVATE_H
#define GPUPROCESS_DISPATCH_PRIVATE_H

#include "config.h"
#include "compiler_private.h"

typedef void (*FunctionPointerType)(void);

#include "dispatch_private_gles2.h"
#include "dispatch_private_desktop.h"

private void
dispatch_init (dispatch_t *dispatch);

#endif /* GPUPROCESS_DISPATCH_PRIVATE_H */
