/* Implemented egl and eglext.h functions.
 * This is the egl functions entry point in the client thread.
 *
 * The server thread is initialized and be ready during eglGetDisplay()
 * and is terminated during eglTerminate() when the current context is
 * None. If a client calls egl functions before eglGetDisplay(), an error
 * is returned.
 * Variables used in the code:
 * (1) active_context - a pointer to the current state that is in
 * global server_states.  If it is NULL, the current client is not in
 * any valid egl context.
 * (2) command_buffer - a pointer to the command buffer that is shared
 * between client thread and server thread.  The client posts command to
 * command buffer, while the server thread reads from the command from
 * the command buffer
 */

#include "config.h"


#include "command_autogen.h"
#include "gles2_api_private.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>


EGLBoolean eglTerminate (EGLDisplay display)
{
    if (! on_client_thread ())
        server_dispatch_table_get_base ()->eglTerminate (NULL, display);

    if (_is_error_state ())
        return EGL_FALSE;

    command_t *command =
        client_get_space_for_command (COMMAND_EGLTERMINATE);
    command_eglterminate_init (command, display);
    client_write_command (command);

    unsigned int token = client_insert_token();
    client_wait_for_token (token);

    client_destroy_thread_local ();

    return EGL_TRUE;
}

EGLBoolean
eglSwapBuffers (EGLDisplay display,
                EGLSurface surface)
{
    command_t *command =
        client_get_space_for_command (COMMAND_EGLSWAPBUFFERS);
    command_eglswapbuffers_init (command, display, surface);
    client_write_command (command);

    unsigned int token = client_insert_token();
    client_wait_for_token (token);

    return ((command_eglswapbuffers_t *)command)->result;
}

EGLBoolean
eglMakeCurrent (EGLDisplay display,
                EGLSurface draw,
                EGLSurface read,
                EGLContext ctx)
{
    if (! on_client_thread ())
        server_dispatch_table_get_base ()->eglMakeCurrent (NULL, display, draw, read, ctx);

    if (_is_error_state ())
        return EGL_FALSE;

    if (display == EGL_NO_DISPLAY || ctx == EGL_NO_CONTEXT)
        return EGL_TRUE;

    command_t *command =
        client_get_space_for_command (COMMAND_EGLMAKECURRENT);
    command_eglmakecurrent_init (command, display, draw, read, ctx);
    client_write_command (command);

    unsigned int token = client_insert_token();
    client_wait_for_token (token);

    return ((command_eglmakecurrent_t *)command)->result;
}
