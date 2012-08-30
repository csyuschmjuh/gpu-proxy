#ifndef GPUPROCESS_GLES2_CLI_PRIVATE_H
#define GPUPROCESS_GLES2_CLI_PRIVATE_H

#include "gpuprocess_egl_states.h"
#include "gpuprocess_compiler_private.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gl_client_states
{
    v_vertex_attrib_list_t        vertex_attribs;

    GLint                        pack_alignment;         /* initial 4 */
    GLint                        unpack_alignment;        /* initial 4 */
  
    /* XXX: for OpenGL, there are more client states for glPixelStore,
     * we have some of them on server, should we also bring to client? 
     */
} gl_client_states_t;

/* thread local client states */
extern __thread  gl_client_states_t cli_states;

/*
gpuprocess_private void 
_gpuprocess_client_init ();
*/

gpuprocess_private void
_gpuprocess_client_destroy ();

#ifdef __cplusplus
}
#endif


#endif /* GPUPROCESS_GLES2_SRV_PRIVATE_H */
