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


EGLBoolean eglTerminate (EGLDisplay dpy)
{
    EGLBoolean result = EGL_FALSE;

    if (! client_get_thread_local ())
        return result;

    /* XXX: post eglTerminate and wait */

    /* FIXME: only if we are in None Context, we will destroy the server
     * according to egl spec.  What happens if we are in valid context
     * and application exit?  Obviously, there are still valid context
     * on the driver side, what about our server thread ?
     */
    if (! client_active_egl_state_available ())
        client_destroy_thread_local ();

    return result;
}

EGLBoolean
eglSwapBuffers (EGLDisplay dpy,
                EGLSurface surface)
{
    EGLBoolean result = EGL_BAD_DISPLAY;
    egl_state_t *egl_state;

    if (! client_active_egl_state_available ())
        return result;

    egl_state = client_get_active_egl_state ();
    if (egl_state->display == EGL_NO_DISPLAY)
        return result;

    if (egl_state->display != dpy)
        goto FINISH;

    if (egl_state->readable != surface || egl_state->drawable != surface) {
        result = EGL_BAD_SURFACE;
        goto FINISH;
    }

    /* XXX: post eglSwapBuffers, no wait */

FINISH:
    return result;
}

EGLBoolean
eglMakeCurrent (EGLDisplay display,
                EGLSurface draw,
                EGLSurface read,
                EGLContext ctx)
{
    EGLBoolean result = EGL_FALSE;
    egl_state_t *egl_state = NULL;

    if (! on_client_thread ())
        server_dispatch_table_get_base ()->eglMakeCurrent (NULL, display, draw, read, ctx);

    if (_is_error_state ())
        return EGL_FALSE;

    /* XXX: we need to pass active_context in command buffer */
    /* we are not in any valid context */
    if (! client_active_egl_state_available ()) {
        if (display == EGL_NO_DISPLAY || ctx == EGL_NO_CONTEXT) {
            return EGL_TRUE;
        }
        else {
            command_t *command =
                client_get_space_for_command (COMMAND_EGLMAKECURRENT);
            command_eglmakecurrent_init (command, display, draw, read, ctx);
            client_write_command (command);
            /* FIXME: update active_context */
            goto WAIT;
        }
    } else {
        if (display == EGL_NO_DISPLAY || ctx == EGL_NO_CONTEXT) {
            command_t *command =
                client_get_space_for_command (COMMAND_EGLMAKECURRENT);
            command_eglmakecurrent_init (command, display, draw, read, ctx);
            client_write_command (command);
            /* FIXME: update active_context */
            return EGL_TRUE;
        }
        else {
            egl_state = client_get_active_egl_state ();
            if (egl_state->display == display &&
                egl_state->context == ctx &&
                egl_state->drawable == draw &&
                egl_state->readable == read) {
                if (egl_state->destroy_dpy)
                    return EGL_FALSE;
                else
                    return EGL_TRUE;
            }
            else {
                command_t *command =
                    client_get_space_for_command (COMMAND_EGLMAKECURRENT);
                command_eglmakecurrent_init (command, display, draw, read, ctx);
                client_write_command (command);
                /* FIXME: update active_context */
                goto WAIT;
            }
        }
    }

    return result;

WAIT:
    {
        unsigned int token = client_insert_token();
        client_wait_for_token (token);
        return EGL_TRUE;
    }
}
