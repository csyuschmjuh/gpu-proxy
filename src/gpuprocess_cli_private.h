#ifndef GPUPROCESS_CLI_PRIVATE_H
#define GPUPROCESS_CLI_PRIVATE_H

#ifdef HAS_GL
#include "glx_states.h"
#elif HAS_GLES2
#include "egl_states.h"
#else
#error "Could not find appropriate backend"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gl_cli_states
{
    int 			count;
    v_vertex_attrib_list_t	embedded_vertex_attribs[32];
    v_vertex_attrib_list_t	*vertex_attribs;

    GLint			pack_alignment; 	/* initial 4 */
    GLint			unpack_alignment;	/* initial 4 */
  
    /* XXX: for OpenGL, there are more client states for glPixelStore,
     * we have some of them on server, should we also bring to client? 
     */
} gl_cli_states_t;

#ifdef __cplusplus
}
#endif

void _virtual_cli_init ();

#endif /* GPUPROCESS_SRV_PRIVATE_H */
