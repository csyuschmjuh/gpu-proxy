#ifndef GPUPROCESS_CLI_PRIVATE_H
#define GPUPROCESS_CLI_PRIVATE_H

#ifdef HAS_GL
#include "gpuprocess_glx_states.h"
#elif HAS_GLES2
#include "gpuprocess_egl_states.h"
#else
#error "Could not find appropriate backend"
#endif

#include "gpuprocess_compiler_private.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gl_cli_states
{
    v_vertex_attrib_list_t	vertex_attribs;

    GLint			pack_alignment; 	/* initial 4 */
    GLint			unpack_alignment;	/* initial 4 */
  
    /* XXX: for OpenGL, there are more client states for glPixelStore,
     * we have some of them on server, should we also bring to client? 
     */
} gl_cli_states_t;

/* thread local client states */
extern __thread  gl_cli_states_t cli_states 
    __attribute__(( tls_model ("initial-exec")));

gpuprocess_private void 
_gpuprocess_cli_init ();

gpuprocess_private void
_gpuprocess_cli_destroy ();

#ifdef __cplusplus
}
#endif


#endif /* GPUPROCESS_SRV_PRIVATE_H */
