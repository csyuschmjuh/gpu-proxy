#include "config.h"

#include "caching_client.h"
#include "caching_client_private.h"
#include "client.h"
#include "command.h"
#include "enum_validation.h"
#include "egl_state.h"
#include "name_handler.h"
#include "types_private.h"
#include <EGL/eglext.h>
#include <EGL/egl.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2.h>
#include <stdlib.h>
#include <string.h>

mutex_static_init (cached_gl_states_mutex);
mutex_static_init (cached_gl_display_list_mutex);

static texture_t *
caching_client_get_default_texture ()
{
    static texture_t tex;
    static bool initialized = false;

    if (! initialized) {
        tex.id = 0;
        tex.data_type = GL_UNSIGNED_BYTE;
        tex.internal_format = GL_RGBA;
        tex.texture_mag_filter = GL_LINEAR;
        tex.texture_min_filter = GL_NEAREST_MIPMAP_LINEAR;
        tex.texture_wrap_s = GL_REPEAT;
        tex.texture_wrap_t = GL_REPEAT;
        tex.texture_3d_wrap_r = GL_REPEAT;
        initialized = true;
    }

    return &tex;
}
static void
caching_client_glSetError (void* client, GLenum error)
{
    egl_state_t *egl_state = client_get_current_state (CLIENT(client));
    if (egl_state && egl_state->active && egl_state->error == GL_NO_ERROR) {
        egl_state->need_get_error = false;
        egl_state->error = error;
    }
}

static bool
caching_client_does_index_overflow (void* client,
                                    GLuint index)
{
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (state->max_vertex_attribs_queried)
        goto FINISH;

    CACHING_CLIENT(client)->super_dispatch.glGetIntegerv (client, GL_MAX_VERTEX_ATTRIBS,
                                                          &state->max_vertex_attribs);
    state->max_vertex_attribs_queried = true;

FINISH:
    if (index < state->max_vertex_attribs)
        return false;

    caching_client_glSetError (client, GL_INVALID_VALUE);
    return true;
}

static void
caching_client_set_needs_get_error (client_t *client)
{
    client_get_current_state (CLIENT (client))->need_get_error = true;
}

static void
caching_client_reset_set_needs_get_error (client_t *client)
{
    client_get_current_state (CLIENT (client))->need_get_error = false;
}

static void
_caching_client_destroy_state (client_t* client,
                               egl_state_t *egl_state)
{
    egl_state->contexts_sharing--;

    /* Wait until we've deleted the children sharing us to delete ourself.*/
    if (egl_state->contexts_sharing > 0)
        return;

    egl_state_t* share_context = egl_state->share_context;
    if (share_context) {
        share_context->contexts_sharing--;
        _caching_client_destroy_state (client, share_context);
    }

    link_list_delete_first_entry_matching_data (cached_gl_states (), egl_state);
}

static egl_state_t *
find_state_with_display_and_context (EGLDisplay display,
                                     EGLContext context)
{
    link_list_t **states = cached_gl_states ();

    link_list_t *current = *states;
    while (current) {
        egl_state_t *state = (egl_state_t *)current->data;
        if (state->context == context && state->display == display) {
            return current->data;
        }
        current = current->next;
    }

    return NULL;
}

static egl_state_t *
_caching_client_get_or_create_state (EGLDisplay dpy,
                                     EGLContext ctx)
{
    /* We should already be holding the cached states mutex. */
    egl_state_t *state = find_state_with_display_and_context(dpy, ctx);
    if (!state) {
        state = egl_state_new (dpy, ctx);
        link_list_append (cached_gl_states (), state, egl_state_destroy);
    }
    return state;
}

/* we should call real eglMakeCurrent() before, and wait for result
 * if eglMakeCurrent() returns EGL_TRUE, then we call this
 */
static void
_caching_client_make_current (client_t *client,
                              EGLDisplay display,
                              EGLSurface drawable,
                              EGLSurface readable,
                              EGLContext context)
{
    mutex_lock (cached_gl_states_mutex);

    /* If we aren't switching to the "none" context, the new_state isn't null. */
    egl_state_t *new_state = NULL;
    if (context != EGL_NO_CONTEXT && display != EGL_NO_DISPLAY) {
        new_state = _caching_client_get_or_create_state (display, context);
        new_state->drawable = drawable;
        new_state->readable = readable;
        new_state->active = true;
    }

    /* We aren't switching contexts, so do nothing. Note that we may have
     * still updated the read and write surfaces above. */
    egl_state_t *current_state = (egl_state_t *) CLIENT(client)->active_state;
    if (current_state == new_state) {
        mutex_unlock (cached_gl_states_mutex);
        return;
    }

    CLIENT(client)->active_state = new_state;

    /* Deactivate the old surface and clean up any previously destroyed bits of it. */
    if (current_state) {
        mutex_lock (cached_gl_display_list_mutex);
        current_state->active = false;
        EGLSurface temp = current_state->readable;

        if (current_state->destroy_read) {
            cached_gl_surface_destroy (display, current_state->readable);
            current_state->readable = EGL_NO_SURFACE;
        }
        if (current_state->destroy_draw) {
            if (temp != current_state->drawable)
                cached_gl_surface_destroy (display, current_state->drawable);
            current_state->drawable = EGL_NO_SURFACE;
        }

        if (current_state->destroy_dpy || current_state->destroy_ctx)
            _caching_client_destroy_state (client, current_state);

        if (current_state->destroy_ctx)
            cached_gl_context_destroy (current_state->display,
                                       current_state->context);
        mutex_unlock (cached_gl_display_list_mutex);
    }

    mutex_unlock (cached_gl_states_mutex);
}

static void
_caching_client_destroy_surface (client_t *client,
                                 EGLDisplay display,
                                 EGLSurface surface)
{
    egl_state_t *state;
    link_list_t **states = cached_gl_states ();
    link_list_t *list = *states;
    link_list_t *current;

    mutex_lock (cached_gl_states_mutex);
    if (! *states) {
        mutex_unlock (cached_gl_states_mutex);
        return;
    }

    while (list != NULL) {
        current = list;
        list = list->next;
        state = (egl_state_t *)current->data;

        if (state->display == display) {
            if (state->readable == surface)
                state->destroy_read = true;
            if (state->drawable == surface)
                state->destroy_draw = true;

            if (state->active == false) {
                if (state->readable == surface &&
                    state->destroy_read == true)
                    state->readable = EGL_NO_SURFACE;
                if (state->drawable == surface &&
                    state->destroy_draw == true)
                    state->drawable = EGL_NO_SURFACE;

                mutex_lock (cached_gl_display_list_mutex);
                cached_gl_surface_destroy (display, surface);
                mutex_unlock (cached_gl_display_list_mutex);
            }

        }
    }

    mutex_unlock (cached_gl_states_mutex);
}

static void
_set_vertex_pointers (vertex_attrib_list_t *list,
                      vertex_attrib_t *new_attrib)
{
    if (! new_attrib->array_enabled)
        return;
    void *pointer = new_attrib->pointer;
    if (! list->first_index_pointer || pointer < list->first_index_pointer->pointer)
        list->first_index_pointer = new_attrib;
    if ( ! list->last_index_pointer || pointer > list->last_index_pointer->pointer)
        list->last_index_pointer = new_attrib;
}

/* GLES2 core profile API */
static void
caching_client_glActiveTexture (void* client,
                                GLenum texture)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;

    if (state->active_texture == texture)
        return;
    if (texture > GL_TEXTURE31 || texture < GL_TEXTURE0) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;

    } else {
        CACHING_CLIENT(client)->super_dispatch.glActiveTexture (client, texture);
        /* FIXME: this maybe not right because this texture may be
         * invalid object, we save here to save time in glGetError()
         */
        state->active_texture = texture;
        /* reset texture binding */
        state->texture_binding[0] = 0;
        state->texture_binding[1] = 0;
        state->texture_binding_3d = 0;
    }
}

static void
caching_client_glBindBuffer (void* client, GLenum target, GLuint buffer)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;
    if (target == GL_ARRAY_BUFFER) {
        if (state->array_buffer_binding == buffer)
            return;
        else {
            CACHING_CLIENT(client)->super_dispatch.glBindBuffer (client, target, buffer);

           /* FIXME: we don't know whether it succeeds or not */
           state->array_buffer_binding = buffer;
        }
    }
    else if (target == GL_ELEMENT_ARRAY_BUFFER) {
        if (state->element_array_buffer_binding == buffer)
            return;
        else {
            CACHING_CLIENT(client)->super_dispatch.glBindBuffer (client, target, buffer);
            //state->need_get_error = true;

           /* FIXME: we don't know whether it succeeds or not */
           state->element_array_buffer_binding = buffer;
        }
    }
    else {
        caching_client_glSetError (client, GL_INVALID_ENUM);
    }
}

static void
caching_client_glBindFramebuffer (void* client, GLenum target, GLuint framebuffer)
{

    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;
    if (target == GL_FRAMEBUFFER &&
        state->framebuffer_binding == framebuffer)
            return;

    if (target != GL_FRAMEBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
    }

    CACHING_CLIENT(client)->super_dispatch.glBindFramebuffer (client, target, framebuffer);
    /* FIXME: should we save it, it will be invalid if the
     * framebuffer is invalid
     */
    state->framebuffer_binding = framebuffer;

    //state->need_get_error = true;
}

static void
caching_client_glBindRenderbuffer (void* client, GLenum target, GLuint renderbuffer)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;

    if (target != GL_RENDERBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
    }

    CACHING_CLIENT(client)->super_dispatch.glBindRenderbuffer (client, target, renderbuffer);
    /* FIXME: should we save it, it will be invalid if the
     * renderbuffer is invalid
     */
    state->renderbuffer_binding = renderbuffer;
}

static void
caching_client_glBindTexture (void* client, GLenum target, GLuint texture)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;
    if (target == GL_TEXTURE_2D &&
        state->texture_binding[0] == texture)
        return;
    else if (target == GL_TEXTURE_CUBE_MAP &&
             state->texture_binding[1] == texture)
        return;
    else if (target == GL_TEXTURE_3D_OES &&
             state->texture_binding_3d == texture)
        return;

    if (! (target == GL_TEXTURE_2D ||
           target == GL_TEXTURE_CUBE_MAP ||
           target == GL_TEXTURE_3D_OES)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    /* look up in cache */
    if (texture != 0 && !egl_state_lookup_cached_texture (state, texture)) {
        name_handler_alloc_name (state->texture_name_handler, texture);
        egl_state_create_cached_texture (state, texture);
    }

    CACHING_CLIENT(client)->super_dispatch.glBindTexture (client, target, texture);

    /* FIXME: do we need to save them ? */
    if (target == GL_TEXTURE_2D)
        state->texture_binding[0] = texture;
    else if (target == GL_TEXTURE_CUBE_MAP)
        state->texture_binding[1] = texture;
    else
        state->texture_binding_3d = texture;
}

static void
caching_client_glBlendColor (void* client, GLclampf red,
                             GLclampf green,
                             GLclampf blue, GLclampf alpha)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;
    if (state->blend_color[0] == red &&
        state->blend_color[1] == green &&
        state->blend_color[2] == blue &&
        state->blend_color[3] == alpha)
        return;

    state->blend_color[0] = red;
    state->blend_color[1] = green;
    state->blend_color[2] = blue;
    state->blend_color[3] = alpha;

    CACHING_CLIENT(client)->super_dispatch.glBlendColor (client, red, green, blue, alpha);
}

static void
caching_client_glBlendEquation (void* client, GLenum mode)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;
    if (state->blend_equation[0] == mode &&
        state->blend_equation[1] == mode)
        return;

    if (! is_valid_Equation (mode)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    state->blend_equation[0] = mode;
    state->blend_equation[1] = mode;

    CACHING_CLIENT(client)->super_dispatch.glBlendEquation (client, mode);
}

static void
caching_client_glBlendEquationSeparate (void* client, GLenum modeRGB, GLenum modeAlpha)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;
    if (state->blend_equation[0] == modeRGB &&
        state->blend_equation[1] == modeAlpha)
        return;

    if (! is_valid_Equation (modeRGB) || ! is_valid_Equation (modeAlpha)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    state->blend_equation[0] = modeRGB;
    state->blend_equation[1] = modeAlpha;

    CACHING_CLIENT(client)->super_dispatch.glBlendEquationSeparate (client, modeRGB, modeAlpha);
}

static void
caching_client_glBlendFunc (void* client, GLenum sfactor, GLenum dfactor)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;
    if (state->blend_src[0] == sfactor &&
        state->blend_src[1] == sfactor &&
        state->blend_dst[0] == dfactor &&
        state->blend_dst[1] == dfactor)
        return;

    if (! is_valid_SrcBlendFactor (sfactor) || ! is_valid_DstBlendFactor (dfactor)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    state->blend_src[0] = state->blend_src[1] = sfactor;
    state->blend_dst[0] = state->blend_dst[1] = dfactor;

    CACHING_CLIENT(client)->super_dispatch.glBlendFunc (client, sfactor, dfactor);
}

static void
caching_client_glBlendFuncSeparate (void* client, GLenum srcRGB, GLenum dstRGB,
                                    GLenum srcAlpha, GLenum dstAlpha)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;
    if (state->blend_src[0] == srcRGB &&
        state->blend_src[1] == srcAlpha &&
        state->blend_dst[0] == dstRGB &&
        state->blend_dst[1] == dstAlpha)
        return;

    if (! is_valid_SrcBlendFactor (srcRGB) || ! is_valid_SrcBlendFactor (srcAlpha) ||
        ! is_valid_DstBlendFactor (dstRGB) || ! is_valid_DstBlendFactor (dstAlpha)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    state->blend_src[0] = srcRGB;
    state->blend_src[1] = srcAlpha;
    state->blend_dst[0] = dstRGB;
    state->blend_dst[0] = dstAlpha;

    CACHING_CLIENT(client)->super_dispatch.glBlendFuncSeparate (client, srcRGB, dstRGB, srcAlpha, dstAlpha);
}

static GLenum
caching_client_glCheckFramebufferStatus (void* client, GLenum target)
{
    GLenum result;

    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return GL_FRAMEBUFFER_UNSUPPORTED;

    if (target != GL_FRAMEBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return GL_INVALID_ENUM;
    }

    result = CACHING_CLIENT(client)->super_dispatch.glCheckFramebufferStatus (client, target);

    /* update framebuffer in cache */
    if (state->framebuffer_binding) {
        framebuffer_t *framebuffer = egl_state_lookup_cached_framebuffer (state, state->framebuffer_binding);
        if (framebuffer)
            framebuffer->complete = (result == GL_FRAMEBUFFER_COMPLETE)? FRAMEBUFFER_COMPLETE: FRAMEBUFFER_INCOMPLETE;
    }

    return result;
}

static void
caching_client_glClear (void* client, GLbitfield mask)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (! (mask & GL_COLOR_BUFFER_BIT ||
           mask & GL_DEPTH_BUFFER_BIT ||
           mask & GL_STENCIL_BUFFER_BIT)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }
    
    if (state->framebuffer_binding) {
        framebuffer_t *framebuffer = egl_state_lookup_cached_framebuffer (state, state->framebuffer_binding);
        if (framebuffer && framebuffer->id && framebuffer->complete == FRAMEBUFFER_INCOMPLETE) {
            caching_client_glSetError (client, GL_INVALID_FRAMEBUFFER_OPERATION);
            return;
        }
    }

    CACHING_CLIENT(client)->super_dispatch.glClear (client, mask);
}

static void
caching_client_glClearColor (void* client, GLclampf red, GLclampf green,
                 GLclampf blue, GLclampf alpha)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;
    if (state->color_clear_value[0] == red &&
        state->color_clear_value[1] == green &&
        state->color_clear_value[2] == blue &&
        state->color_clear_value[3] == alpha)
        return;
    
    if (state->framebuffer_binding) {
        framebuffer_t *framebuffer = egl_state_lookup_cached_framebuffer (state, state->framebuffer_binding);
        if (framebuffer && framebuffer->id && framebuffer->complete == FRAMEBUFFER_INCOMPLETE) {
            caching_client_glSetError (client, GL_INVALID_FRAMEBUFFER_OPERATION);
            return;
        }
    }

    state->color_clear_value[0] = red;
    state->color_clear_value[1] = green;
    state->color_clear_value[2] = blue;
    state->color_clear_value[3] = alpha;

    CACHING_CLIENT(client)->super_dispatch.glClearColor (client, red, green, blue, alpha);
}

static void
caching_client_glClearDepthf (void* client, GLclampf depth)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;
    if (state->depth_clear_value == depth)
        return;
    
    if (state->framebuffer_binding) {
        framebuffer_t *framebuffer = egl_state_lookup_cached_framebuffer (state, state->framebuffer_binding);
        if (framebuffer && framebuffer->id && framebuffer->complete == FRAMEBUFFER_INCOMPLETE) {
            caching_client_glSetError (client, GL_INVALID_FRAMEBUFFER_OPERATION);
            return;
        }
    }

    state->depth_clear_value = depth;

    CACHING_CLIENT(client)->super_dispatch.glClearDepthf (client, depth);
}

static void
caching_client_glClearStencil (void* client, GLint s)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;
    if (state->stencil_clear_value == s)
        return;
    
    if (state->framebuffer_binding) {
        framebuffer_t *framebuffer = egl_state_lookup_cached_framebuffer (state, state->framebuffer_binding);
        if (framebuffer && framebuffer->id && framebuffer->complete == FRAMEBUFFER_INCOMPLETE) {
            caching_client_glSetError (client, GL_INVALID_FRAMEBUFFER_OPERATION);
            return;
        }
    }

    state->stencil_clear_value = s;
    CACHING_CLIENT(client)->super_dispatch.glClearStencil (client, s);
}

static void
caching_client_glColorMask (void* client, GLboolean red, GLboolean green,
                GLboolean blue, GLboolean alpha)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;
    if (state->color_writemask[0] == red &&
        state->color_writemask[1] == green &&
        state->color_writemask[2] == blue &&
        state->color_writemask[3] == alpha)
        return;

    state->color_writemask[0] = red;
    state->color_writemask[1] = green;
    state->color_writemask[2] = blue;
    state->color_writemask[3] = alpha;

    CACHING_CLIENT(client)->super_dispatch.glColorMask (client, red, green, blue, alpha);
}

static void 
caching_client_glCopyTexImage2D (void *client,
                                 GLenum target,
                                 GLint level,
                                 GLenum internalformat,
                                 GLint x,
                                 GLint y,
                                 GLsizei width,
                                 GLsizei height,
                                 GLint border)
{
    GLuint tex = 0;
    texture_t *texture = NULL;
    framebuffer_t *framebuffer = NULL;

    INSTRUMENT();
    
    egl_state_t *state = client_get_current_state (CLIENT (client));

    /* call dispatch */
    CACHING_CLIENT(client)->super_dispatch.glCopyTexImage2D (client, 
                                                              target,
                                                              level,
                                                              internalformat,
                                                              x, y,
                                                              width, height,
                                                              border);
    caching_client_set_needs_get_error (CLIENT (client));

    if (target == GL_TEXTURE_2D)
        tex = state->texture_binding[0];
    else if (target == GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
             target == GL_TEXTURE_CUBE_MAP_NEGATIVE_X ||
             target == GL_TEXTURE_CUBE_MAP_POSITIVE_Y ||
             target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y ||
             target == GL_TEXTURE_CUBE_MAP_POSITIVE_Z ||
             target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)
        tex = state->texture_binding[1];

    if (tex) 
        texture = egl_state_lookup_cached_texture (state, tex);

    if (texture)
        framebuffer = egl_state_lookup_cached_framebuffer (state, texture->framebuffer_id);

    if (framebuffer && framebuffer->id)
        framebuffer->complete = FRAMEBUFFER_COMPLETE_UNKNOWN;
}

static void 
caching_client_glCompressedTexImage2D (void *client,
                                       GLenum target,
                                       GLint level,
                                       GLenum internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLint border,
                                       GLsizei imageSize,
                                       const GLvoid *data)
{
    GLuint tex = 0;
    texture_t *texture = NULL;
    framebuffer_t *framebuffer = NULL;

    INSTRUMENT();
    
    egl_state_t *state = client_get_current_state (CLIENT (client));

    /* call dispatch */
    CACHING_CLIENT(client)->super_dispatch.glCompressedTexImage2D (client, 
                                                           target,
                                                           level,
                                                           internalformat,
                                                           width, height,
                                                           border,
                                                           imageSize,
                                                           data);
    caching_client_set_needs_get_error (CLIENT (client));

    if (target == GL_TEXTURE_2D)
        tex = state->texture_binding[0];
    else if (target == GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
             target == GL_TEXTURE_CUBE_MAP_NEGATIVE_X ||
             target == GL_TEXTURE_CUBE_MAP_POSITIVE_Y ||
             target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y ||
             target == GL_TEXTURE_CUBE_MAP_POSITIVE_Z ||
             target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)
        tex = state->texture_binding[1];

    if (tex) 
        texture = egl_state_lookup_cached_texture (state, tex);

    if (texture)
        framebuffer = egl_state_lookup_cached_framebuffer (state, texture->framebuffer_id);

    if (framebuffer && framebuffer->id)
        framebuffer->complete = FRAMEBUFFER_COMPLETE_UNKNOWN;

}

static GLuint
caching_client_glCreateProgram (void* client)
{
    INSTRUMENT();
    GLuint result = 0;
    egl_state_t *state = client_get_current_state (CLIENT (client));
    command_t *command = NULL;

    if (!state)
        return 0;

    name_handler_alloc_names (state->shader_objects_name_handler, 1, &result);
    command = client_get_space_for_command (COMMAND_GLCREATEPROGRAM);
    command_glcreateprogram_init (command);
    ((command_glcreateprogram_t *)command)->result = result;

    client_run_command_async (command);

    if (result == 0) {
        caching_client_set_needs_get_error (CLIENT (client));
        return 0;
    }

    egl_state_create_cached_program (state, result);
    return result;
}

static program_t *
egl_state_lookup_cached_program_err (void *client,
                                     GLuint shader_object_id,
                                     GLenum program_error)
{
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return 0;

    program_t *cached_program = (program_t*) egl_state_lookup_cached_shader_object (state, shader_object_id);
    if (!cached_program) {
        caching_client_glSetError (client, program_error);
        return 0;
    }

    if (cached_program->base.type != SHADER_OBJECT_PROGRAM) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return 0;
    }

    return cached_program;
}

static void
caching_client_glDeleteProgram (void *client,
                                GLuint program)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;

    program_t *cached_program = (program_t*) egl_state_lookup_cached_shader_object (state, program);
    if (!cached_program) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glDeleteProgram (client, program);

    if (cached_program->base.id == state->current_program) {
        cached_program->mark_for_deletion = true;
        return;
    }

    egl_state_destroy_cached_shader_object (state, &cached_program->base);
}

static GLuint
caching_client_glCreateShader (void* client, GLenum shaderType)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return 0;

    if (! is_valid_ShaderType (shaderType)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return 0;
    }

    GLuint result = 0;
    command_t *command = client_get_space_for_command (COMMAND_GLCREATESHADER);

    name_handler_alloc_names (state->shader_objects_name_handler, 1, &result);
    command_glcreateshader_init (command, shaderType);
    ((command_glcreateshader_t *)command)->result = result;

    client_run_command_async (command);

    if (result == 0)
        caching_client_set_needs_get_error (CLIENT (client));
    else
        egl_state_create_cached_shader (state, result);

    return result;
}

static void
caching_client_glShaderSource (void *client, GLuint shader, GLsizei count,
                               const GLchar **string, const GLint *length)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
	return;

    if (count <= 0 || ! string) {
	caching_client_glSetError (client, GL_INVALID_VALUE);
	return;
    }

    GLint *caching_client_length = NULL;
    if (length != NULL) {
        size_t lengths_size = sizeof (GLint *) * count;
        caching_client_length = malloc (lengths_size);
        memcpy (caching_client_length, length, lengths_size);
    }

    char **caching_client_string;
    caching_client_string = malloc (sizeof (GLchar *) * count);

    unsigned i = 0;
    int n;
    bool null_terminated = false;

    for (i = 0; i < count; i++) {
        if (! string[i]) {
            if (caching_client_length)
                free (caching_client_length);
            for (n = 0; n < i; n++)
                free (caching_client_string[n]);
            free (caching_client_string);
            caching_client_glSetError (client, GL_INVALID_OPERATION);
	    return;
        }

        size_t string_length = length ? length[i] : strlen (string[i]);
        if (string_length < 0)
            string_length = strlen (string[i]);

        if (!length || length[i] < 0)
            null_terminated = true;
        else
            null_terminated = false;

        if (null_terminated)
            caching_client_string[i] = malloc (string_length + 1);
        else
            caching_client_string[i] = malloc (string_length);

        memcpy (caching_client_string[i], string[i], string_length);
        if (null_terminated)
            caching_client_string[i][string_length] = 0;
    }

    caching_client_set_needs_get_error (CLIENT (client));

    CACHING_CLIENT(client)->super_dispatch.glShaderSource(client, shader, count,
                                                          (const char **)caching_client_string,
                                                          caching_client_length);
}

static void
caching_client_glAttachShader (void *client, GLuint program, GLuint shader)
{
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;

    program_t *cached_program = (program_t*) egl_state_lookup_cached_program_err (client, program, GL_INVALID_VALUE);
    if (!cached_program)
        return;

    shader_object_t *cached_shader = (shader_object_t*) egl_state_lookup_cached_shader_object (state, shader);
    if (!cached_shader) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    if (cached_shader->type == SHADER_OBJECT_PROGRAM) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return;
    }

    GLuint *shader_data = (GLuint *) malloc (sizeof(GLuint));
    *shader_data = shader;
    link_list_append (&cached_program->attached_shaders, shader_data, free);
    CACHING_CLIENT(client)->super_dispatch.glAttachShader (client, program, shader);
}

static void
caching_client_glGetAttachedShaders (void    *client,
                                     GLuint   program,
                                     GLsizei  maxCount,
                                     GLsizei *count,
                                     GLuint  *shaders)
{
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;

    program_t *cached_program = egl_state_lookup_cached_program_err (client, program, GL_INVALID_VALUE);
    if (!cached_program)
        return;

    if (maxCount < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    link_list_t *current = cached_program->attached_shaders;
    GLsizei attached_shaders_size = 0;
    while (current) {
        if (attached_shaders_size > maxCount)
            break;
        if (shaders) {
            GLuint *shader = (GLuint *)current->data;
            shaders[attached_shaders_size] = *shader;
        }
        ++attached_shaders_size;

        current = current->next;
    }
    if (count)
        *count = attached_shaders_size;
}

static void
caching_client_glCullFace (void* client, GLenum mode)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;
    if (state->cull_face_mode == mode)
        return;

    if (! is_valid_FaceType (mode)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    state->cull_face_mode = mode;
    CACHING_CLIENT(client)->super_dispatch.glCullFace (client, mode);
}

static void
caching_client_glDeleteBuffers (void* client, GLsizei n, const GLuint *buffers)
{
    int count;
    int i, j;

    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;
    vertex_attrib_list_t *attrib_list = &state->vertex_attribs;
    vertex_attrib_t *attribs = attrib_list->attribs;
    count = attrib_list->count;

    if (n < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glDeleteBuffers (client, n, buffers);

    name_handler_delete_names (state->buffer_name_handler, n, buffers);

    /* check array_buffer_binding and element_array_buffer_binding */
    for (i = 0; i < n; i++) {
        if (buffers[i] == state->array_buffer_binding)
            state->array_buffer_binding = 0;
        else if (buffers[i] == state->element_array_buffer_binding)
            state->element_array_buffer_binding = 0;
    }

    /* update client state */
    if (count == 0)
        return;

    for (i = 0; i < n; i++) {
        if (attribs[0].array_buffer_binding == buffers[i]) {
            for (j = 0; j < count; j++) {
                attribs[j].array_buffer_binding = 0;
            }
            break;
        }
    }
}

static void
caching_client_glDeleteFramebuffers (void* client, GLsizei n, const GLuint *framebuffers)
{
    framebuffer_t *framebuffer = NULL;
    texture_t *texture = NULL;
    renderbuffer_t *renderbuffer = NULL;

    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;

    if (n < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    name_handler_delete_names (state->framebuffer_name_handler, n, framebuffers);

    CACHING_CLIENT(client)->super_dispatch.glDeleteFramebuffers (client, n, framebuffers);

    int i;
    for (i = 0; i < n; i++) {
        if (framebuffers[i] == 0)
            continue;

        framebuffer = egl_state_lookup_cached_framebuffer (state, framebuffers[i]);
        if (framebuffer) {
            if (! framebuffer->attached_image) {
                texture = egl_state_lookup_cached_texture (state, 
                                                           framebuffer->attached_image);
                if (texture)
                    texture->framebuffer_id = 0;
            }

            if (! framebuffer->attached_color_buffer) {
                renderbuffer = egl_state_lookup_cached_renderbuffer (state,
                                       framebuffer->attached_color_buffer);
                if (renderbuffer)
                    renderbuffer->framebuffer_id = 0;
            }
            if (framebuffer->attached_stencil_buffer != framebuffer->attached_color_buffer && framebuffer->attached_stencil_buffer) {
                renderbuffer = egl_state_lookup_cached_renderbuffer (state,
                                       framebuffer->attached_stencil_buffer);
                if (renderbuffer)
                    renderbuffer->framebuffer_id = 0;
            }
            if (framebuffer->attached_depth_buffer != framebuffer->attached_depth_buffer && framebuffer->attached_depth_buffer) {
                renderbuffer = egl_state_lookup_cached_renderbuffer (state,
                                       framebuffer->attached_depth_buffer);
                if (renderbuffer)
                    renderbuffer->framebuffer_id = 0;
            }
        }
        if (state->framebuffer_binding == framebuffers[i]) {
            state->framebuffer_binding = 0;
        }
    }
}

static void
caching_client_glDeleteRenderbuffers (void* client, GLsizei n, const GLuint *renderbuffers)
{
    renderbuffer_t *renderbuffer = NULL;
    framebuffer_t *framebuffer = NULL;
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;

    if (n < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    name_handler_delete_names (state->renderbuffer_name_handler, n, renderbuffers);

    CACHING_CLIENT(client)->super_dispatch.glDeleteRenderbuffers (client, n, renderbuffers);
    int i;
    for (i = 0; i < n; i++) {
        if (renderbuffers[i] == 0)
            continue;

        renderbuffer = egl_state_lookup_cached_renderbuffer (state, renderbuffers[i]);
        if (renderbuffer) 
            framebuffer = egl_state_lookup_cached_framebuffer (state, renderbuffer->framebuffer_id);
        if (framebuffer && renderbuffer->framebuffer_id) {
            framebuffer->complete = FRAMEBUFFER_COMPLETE_UNKNOWN;
            if (framebuffer->attached_color_buffer == renderbuffers[i])
                framebuffer->attached_color_buffer = 0;
            if (framebuffer->attached_stencil_buffer == renderbuffers[i])
                framebuffer->attached_stencil_buffer = 0;
            if (framebuffer->attached_depth_buffer == renderbuffers[i])
                framebuffer->attached_depth_buffer = 0;
        }

        if (state->renderbuffer_binding == renderbuffers[i]) {
            state->renderbuffer_binding = 0;
        }
    }
}

static void
caching_client_glDeleteTextures (void* client, GLsizei n, const GLuint *textures)
{
    int i;

    texture_t *tex = NULL;
    framebuffer_t *framebuffer = NULL;

    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (!state)
        return;

    if (n < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glDeleteTextures (client, n, textures);

    for (i = 0; i < n; i++) {
        if (textures[i] == 0)
            continue;

        tex = egl_state_lookup_cached_texture (state, textures[i]);
        if (tex) 
            framebuffer = egl_state_lookup_cached_framebuffer (state, tex->framebuffer_id);
        if (framebuffer && tex->framebuffer_id) {
            framebuffer->complete = FRAMEBUFFER_COMPLETE_UNKNOWN;
            framebuffer->attached_image = 0;
        }

        egl_state_delete_cached_texture (state, textures[i]);

        if (state->texture_binding[0] == textures[i])
            state->texture_binding[0] = 0;
        else if (state->texture_binding[1] == textures[i])
            state->texture_binding[1] = 0;
        else if (state->texture_binding_3d == textures[i])
            state->texture_binding_3d = 0;
        if (state->active_texture == textures[i])
            state->active_texture = 0;

    }
}

static void
caching_client_glDepthFunc (void* client, GLenum func)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    if (state->depth_func == func)
        return;

    if (! is_valid_CmpFunction (func)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    state->depth_func = func;
    CACHING_CLIENT(client)->super_dispatch.glDepthFunc (client, func);
}

static void
caching_client_glDepthMask (void* client, GLboolean flag)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    if (state->depth_writemask == flag)
        return;

    state->depth_writemask = flag;
    CACHING_CLIENT(client)->super_dispatch.glDepthMask (client, flag);
}

static void
caching_client_glDepthRangef (void* client, GLclampf nearVal, GLclampf farVal)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    if (state->depth_range[0] == nearVal &&
        state->depth_range[1] == farVal)
        return;

    state->depth_range[0] = nearVal;
    state->depth_range[1] = farVal;

    CACHING_CLIENT(client)->super_dispatch.glDepthRangef (client, nearVal, farVal);
}

void
caching_client_glSetCap (void* client, GLenum cap, GLboolean enable)
{
    bool needs_call = false;

    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    switch (cap) {
    case GL_BLEND:
        if (state->blend != enable) {
            state->blend = enable;
            needs_call = true;
        }
        break;
    case GL_CULL_FACE:
        if (state->cull_face != enable) {
            state->cull_face = enable;
            needs_call = true;
        }
        break;
    case GL_DEPTH_TEST:
        if (state->depth_test != enable) {
            state->depth_test = enable;
            needs_call = true;
        }
        break;
    case GL_DITHER:
        if (state->dither != enable) {
            state->dither = enable;
            needs_call = true;
        }
        break;
    case GL_POLYGON_OFFSET_FILL:
        if (state->polygon_offset_fill != enable) {
            state->polygon_offset_fill = enable;
            needs_call = true;
        }
        break;
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
        if (state->sample_alpha_to_coverage != enable) {
            state->sample_alpha_to_coverage = enable;
            needs_call = true;
        }
        break;
    case GL_SAMPLE_COVERAGE:
        if (state->sample_coverage != enable) {
            state->sample_coverage = enable;
            needs_call = true;
        }
        break;
    case GL_SCISSOR_TEST:
        if (state->scissor_test != enable) {
            state->scissor_test = enable;
            needs_call = true;
        }
        break;
    case GL_STENCIL_TEST:
        if (state->stencil_test != enable) {
            state->stencil_test = enable;
            needs_call = true;
        }
        break;
    default:
        needs_call = true;
        //state->need_get_error = true;
        break;
    }

    if (needs_call) {
        if (enable) {
            CACHING_CLIENT(client)->super_dispatch.glEnable (client, cap);
        } else {
            CACHING_CLIENT(client)->super_dispatch.glDisable (client, cap);
        }
    }
}

static void
caching_client_glDisable (void* client, GLenum cap)
{
    caching_client_glSetCap (client, cap, GL_FALSE);
}

static void
caching_client_glEnable (void* client, GLenum cap)
{
    caching_client_glSetCap (client, cap, GL_TRUE);
}

static void
caching_client_glSetVertexAttribArray (void* client,
                                       GLuint index,
                                       egl_state_t *state,
                                       GLboolean enable)
{
    vertex_attrib_list_t *attrib_list = &state->vertex_attribs;
    vertex_attrib_t *attribs = attrib_list->attribs;
    int count = attrib_list->count;
    int i, found_index = -1;

    GLint bound_buffer = 0;

    /* if vertex_array_binding, we don't do on client */
    if ((state->vertex_array_binding)) {
        if (enable) {
           CACHING_CLIENT(client)->super_dispatch.glEnableVertexAttribArray (client, index);
        }
        else {
           CACHING_CLIENT(client)->super_dispatch.glDisableVertexAttribArray (client, index);
        }
        return;
    }

    /* look into client state */
    for (i = 0; i < count; i++) {
        if (attribs[i].index == index) {
            if (attribs[i].array_enabled == enable)
                return;
            else {
                found_index = i;
                attribs[i].array_enabled = enable;
                break;
            }
        }
    }

    /* gles2 spec says at least 8 */
    if (caching_client_does_index_overflow (client, index))
        return;

    if (enable == GL_FALSE) {
       CACHING_CLIENT(client)->super_dispatch.glDisableVertexAttribArray (client, index);
    } else {
       CACHING_CLIENT(client)->super_dispatch.glEnableVertexAttribArray (client, index);
    }

    bound_buffer = state->array_buffer_binding;

    /* update client state */
    if (found_index != -1) {
        if (! bound_buffer) {
            if (enable) {
                attrib_list->enabled_count++;
                _set_vertex_pointers (attrib_list, &attribs[found_index]);
            }
            else {
                if (&attribs[found_index] == attrib_list->first_index_pointer)
                    attrib_list->first_index_pointer = 0;
                if (&attribs[found_index] == attrib_list->last_index_pointer)
                    attrib_list->last_index_pointer = 0;

                attrib_list->enabled_count = 0;
                for (i = 0; i < attrib_list->count; i++) {
                    if (attribs[i].array_enabled && !attribs[i].array_buffer_binding) {
                        attrib_list->enabled_count++;
                        _set_vertex_pointers (attrib_list, &attribs[i]);
                    }
                }
            }
        }
        return;
    }

    if (i < NUM_EMBEDDED) {
        attribs[i].array_enabled = enable;
        attribs[i].index = index;
        attribs[i].size = 0;
        attribs[i].stride = 0;
        attribs[i].type = GL_FLOAT;
        attribs[i].array_normalized = GL_FALSE;
        attribs[i].pointer = NULL;
        attribs[i].data = NULL;
        attribs[i].array_buffer_binding = bound_buffer;
        attrib_list->count ++;
    }
    else {
        vertex_attrib_t *new_attribs =
            (vertex_attrib_t *)malloc (sizeof (vertex_attrib_t) * (count + 1));

        memcpy (new_attribs, attribs, (count+1) * sizeof (vertex_attrib_t));
        if (attribs != attrib_list->embedded_attribs)
            free (attribs);

        new_attribs[count].array_enabled = enable;
        new_attribs[count].index = index;
        new_attribs[count].size = 0;
        new_attribs[count].stride = 0;
        new_attribs[count].type = GL_FLOAT;
        new_attribs[count].array_normalized = GL_FALSE;
        new_attribs[count].pointer = NULL;
        new_attribs[count].data = NULL;
        new_attribs[count].array_buffer_binding = bound_buffer;
        attrib_list->attribs = new_attribs;
        attrib_list->count ++;
    }
}

static inline int
_get_data_size (GLenum type)
{
    if (type == GL_BYTE || type == GL_UNSIGNED_BYTE)
        return sizeof (char);
    else if (type == GL_SHORT || type == GL_UNSIGNED_SHORT)
        return sizeof (short);
    else if (type == GL_FLOAT)
        return sizeof (float);
    else if (type == GL_FIXED)
        return sizeof (int);

    return 0;
}

static void
caching_client_glDisableVertexAttribArray (void* client, GLuint index)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    caching_client_glSetVertexAttribArray (client, index, state, GL_FALSE);
}

static void
caching_client_glEnableVertexAttribArray (void* client, GLuint index)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    caching_client_glSetVertexAttribArray (client, index, state, GL_TRUE);
}

static char *
_create_data_array (vertex_attrib_t *attrib, int count)
{
    int i;
    char *data = NULL;
    int size = 0;

    INSTRUMENT();

    size = _get_data_size (attrib->type);

    if (size == 0)
        return NULL;

    data = (char *)malloc (size * count * attrib->size);

    if (attrib->size * size == attrib->stride || attrib->stride == 0)
        memcpy (data, attrib->pointer, attrib->size * size * count);
    else {
        for (i = 0; i < count; i++)
            memcpy (data + i * attrib->size * size, attrib->pointer + attrib->stride * i, attrib->size * size);
    }

    return data;
}

static void
caching_client_clear_attribute_list_data (client_t *client)
{
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    vertex_attrib_list_t *attrib_list = &state->vertex_attribs;

    int i = -1;
    for (i = 0; i < attrib_list->count; i++)
        attrib_list->attribs[i].data = NULL;
}

static bool
client_has_vertex_attrib_array_set (client_t *client)
{
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return false;
    vertex_attrib_list_t *attrib_list = &state->vertex_attribs;
    vertex_attrib_t *attribs = attrib_list->attribs;

    int i;
    for (i = 0; i < attrib_list->count; i++) {
        if (attribs[i].array_enabled || attribs[i].array_buffer_binding)
            return true;
    }
    return false;
}

static size_t
caching_client_vertex_attribs_array_size (client_t *client,
                                          size_t count)
{
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return 0;
    vertex_attrib_list_t *attrib_list = &state->vertex_attribs;

    vertex_attrib_t *last_pointer = attrib_list->last_index_pointer;
    if (! last_pointer)
        return 0;
    if (! last_pointer->array_enabled || last_pointer->array_buffer_binding)
        return 0;

    int data_size = _get_data_size (last_pointer->type);
    if (data_size == 0)
        return 0;

    unsigned int stride = last_pointer->stride ? last_pointer->stride :
                                                 (last_pointer->size * data_size);
    return stride * count + (char *)last_pointer->pointer - (char*) attrib_list->first_index_pointer->pointer;
}

static void
caching_client_setup_vertex_attrib_pointer_if_necessary (client_t *client,
                                                         size_t count,
                                                         link_list_t **allocated_data_arrays,
                                                         command_t **command,
                                                         size_t *array_size,
                                                         size_t index_array_size)
{
    int i = 0;

    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    vertex_attrib_list_t *attrib_list = &state->vertex_attribs;
    vertex_attrib_t *attribs = attrib_list->attribs;

    size_t draw_command_size = index_array_size ?
        command_get_size (COMMAND_GLDRAWELEMENTS) :
        command_get_size (COMMAND_GLDRAWARRAYS);
    size_t commands_size =
        command_get_size (COMMAND_GLVERTEXATTRIBPOINTER) * attrib_list->enabled_count +
        draw_command_size;

    *array_size = caching_client_vertex_attribs_array_size (client, count);
    if (!*array_size)
        return;

    bool fits_in_one_array = *array_size < ATTRIB_BUFFER_SIZE;
    command_t *glDraw_command = NULL;
    if (fits_in_one_array) {
        *command = client_get_space_for_size (client, commands_size + *array_size + index_array_size);

        glDraw_command = (command_t *)((char*)*command +
                                       command_get_size (COMMAND_GLVERTEXATTRIBPOINTER) * attrib_list->enabled_count);

        memcpy ((char *)*command + commands_size,
                attrib_list->first_index_pointer->pointer,
                *array_size);
    }

    int attrib_count = 0;
    for (i = 0; i < attrib_list->count; i++) {
        command_t *attrib_command = NULL;

        if (! attribs[i].array_enabled || attribs[i].array_buffer_binding) {
            continue;
        }

        if (fits_in_one_array) {
            attribs[i].data = (char *)attribs[i].pointer - (char *)attrib_list->first_index_pointer->pointer +
                (char *)*command + commands_size;
            attrib_command = (command_t *)((char *)*command +
                                           command_get_size (COMMAND_GLVERTEXATTRIBPOINTER) * attrib_count);
            attrib_command->type = COMMAND_GLVERTEXATTRIBPOINTER;
            attrib_command->size = command_get_size (COMMAND_GLVERTEXATTRIBPOINTER);
            attrib_command->token = 0;
        } else {
            attribs[i].data = _create_data_array (&attribs[i], count);
            if (! attribs[i].data)
                continue;
            link_list_prepend (allocated_data_arrays, attribs[i].data, free);
            attrib_command = client_get_space_for_command (COMMAND_GLVERTEXATTRIBPOINTER);
        }

        if (fits_in_one_array)
            command_glvertexattribpointer_init (attrib_command,
                                            attribs[i].index,
                                            attribs[i].size,
                                            attribs[i].type,
                                            attribs[i].array_normalized,
                                            attribs[i].stride,
                                            (const void *)attribs[i].data);
        else
            command_glvertexattribpointer_init (attrib_command,
                                            attribs[i].index,
                                            attribs[i].size,
                                            attribs[i].type,
                                            attribs[i].array_normalized,
                                            0,
                                            (const void *)attribs[i].data);
        client_run_command_async (attrib_command);

        attrib_count++;
    }

    if (fits_in_one_array)
        *command = glDraw_command;
}

static void
caching_client_glDrawArrays (void* client,
                             GLenum mode,
                             GLint first,
                             GLsizei count)
{
    framebuffer_t *framebuffer = NULL;

    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (! is_valid_DrawMode (mode)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        caching_client_clear_attribute_list_data (CLIENT(client));
        return;
    }

    if ((count < 0) || (first < 0)) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        caching_client_clear_attribute_list_data (CLIENT(client));
        return;
    }

    if (count == 0) {
        caching_client_clear_attribute_list_data (CLIENT(client));
        return;
    }

    /* If vertex array binding is 0 and we have not bound a vertex attrib pointer, we
     * don't need to do anything. */
    if (! state->vertex_array_binding && ! client_has_vertex_attrib_array_set (CLIENT (client))) {
        caching_client_clear_attribute_list_data (CLIENT(client));
        return;
    }
    
    if (state->framebuffer_binding) {
        framebuffer_t *framebuffer = egl_state_lookup_cached_framebuffer (state, state->framebuffer_binding);
        if (framebuffer && framebuffer->id && framebuffer->complete == FRAMEBUFFER_INCOMPLETE) {
            caching_client_clear_attribute_list_data (CLIENT(client));
            caching_client_glSetError (client, GL_INVALID_FRAMEBUFFER_OPERATION);
            return;
        }
    }

    link_list_t *arrays_to_free = NULL;
    command_t *command = NULL;
    size_t array_size = 0;
    if (! state->vertex_array_binding) {
        size_t true_count = first > 0 ? first + count : count;
        caching_client_setup_vertex_attrib_pointer_if_necessary (CLIENT(client),
                                                                 true_count,
                                                                 &arrays_to_free,
                                                                 &command,
                                                                 &array_size,
                                                                 0);
    }

    if (!command)
        command = client_get_space_for_command (COMMAND_GLDRAWARRAYS);
    else {
        command->type = COMMAND_GLDRAWARRAYS;
        command->size = command_get_size (COMMAND_GLDRAWARRAYS) + array_size;
        command->token = 0;
    }

    command_gldrawarrays_init (command, mode, first, count);
    ((command_gldrawarrays_t *) command)->arrays_to_free = arrays_to_free;
     client_run_command_async (command);

    caching_client_clear_attribute_list_data (CLIENT(client));
    if (framebuffer && framebuffer->id && framebuffer->complete == FRAMEBUFFER_COMPLETE_UNKNOWN)
        caching_client_set_needs_get_error (CLIENT (client));
}

#ifdef __ARM_NEON__
#include <arm_neon.h>
union __char_result {
    unsigned char     chars[16];
    uint8x16_t result;
};

union __short_result {
    uint16x8_t result;
    unsigned short shorts[8];
};

static size_t
_get_elements_count (GLenum type, const GLvoid *indices, GLsizei count)
{
    uint8x16_t *char_indices;
    unsigned char *char_idx;
    uint16x8_t *short_indices;
    unsigned short *short_idx;

    size_t elements_count = 0;
    int i;
    int num;
    int remain;
    int j;

    union __short_result short_result = { {0, 0, 0, 0,
                                           0, 0, 0, 0} };
    union __char_result char_result = { {0, 0, 0, 0,
                                        0, 0, 0, 0,
                                        0, 0, 0, 0,
                                        0, 0, 0, 0 } };

    INSTRUMENT();

    if (type == GL_UNSIGNED_BYTE) {
        num = count / 16;
        remain = count - num * 16;
    }
    else {
        num = count / 8;
        remain = count - num * 8;
    }

    if (type == GL_UNSIGNED_BYTE) {
        char_indices = (uint8x16_t *)indices;
        for (i = 0; i < num ; i++) {
            char_result.result = vmaxq_u8 (*(char_indices + i),
                                           char_result.result);
        }
        char_idx = (unsigned char *)char_result.chars;
        for (j = 0; j < 16; j++) {
            if (elements_count < (size_t)char_idx[j])
                elements_count = (size_t)char_idx[j];
        }
        char_idx = (unsigned char *)indices + num * 16;
        for (i = 0; i < remain; i++) {
            if ((size_t) char_idx[i] > elements_count)
                elements_count = (size_t) char_idx[i];
        }
    }
    else {
        short_indices = (uint16x8_t *)indices;
        for (i = 0; i < num ; i++) {
            short_result.result = vmaxq_u16 (*(short_indices + i),
                                             short_result.result);
        }
        short_idx = (unsigned short *)short_result.shorts;
        for (j = 0; j < 8; j++) {
            if (elements_count < (size_t)short_idx[j])
                elements_count = (size_t)short_idx[j];
        }
        short_idx = (unsigned short *)indices + num * 8;
        for (i = 0; i < (size_t) remain; i++) {
            if ((size_t)short_idx[i] > elements_count)
                elements_count = (size_t)short_idx[i];
        }
    }

    return elements_count + 1;
}
#else
static size_t
_get_elements_count (GLenum type, const GLvoid *indices, GLsizei count)
{
    unsigned char *char_indices = NULL;
    unsigned short *short_indices = NULL;
    size_t elements_count = 0;
    size_t i;

    INSTRUMENT();

    if (type == GL_UNSIGNED_BYTE) {
        char_indices = (unsigned char *)indices;
        for (i = 0; i < (size_t) count; i++) {
            if ((size_t) char_indices[i] > elements_count)
                elements_count = (size_t) char_indices[i];
        }
    }
    else {
        short_indices = (unsigned short *)indices;
        for (i = 0; i < (size_t) count; i++) {
            if ((size_t)short_indices[i] > elements_count)
                elements_count = (size_t)short_indices[i];
        }
    }

    return elements_count + 1;
}
#endif

static size_t
calculate_index_array_size (GLenum type,
                            int count)
{
    if (type == GL_UNSIGNED_BYTE)
       return sizeof (char) * count;
    if (type == GL_UNSIGNED_SHORT)
        return sizeof (unsigned short) * count;
    return 0;
}

static void
caching_client_glDrawElements (void* client,
                               GLenum mode,
                               GLsizei count,
                               GLenum type,
                               const GLvoid *indices)
{
    framebuffer_t *framebuffer = NULL;

    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    if (! is_valid_DrawMode (mode)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        caching_client_clear_attribute_list_data (CLIENT(client));
        return;
    }
    if (! (type == GL_UNSIGNED_BYTE  || 
           type == GL_UNSIGNED_SHORT ||
           (state->supports_element_index_uint && type == GL_UNSIGNED_INT))) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        caching_client_clear_attribute_list_data (CLIENT(client));
        return;
    }

    if (count < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        caching_client_clear_attribute_list_data (CLIENT(client));
        return;
    }

    if (count == 0) {
        caching_client_clear_attribute_list_data (CLIENT(client));
        return;
    }

    /* If we have not bound any attribute data then do not actually execute anything. */
    if (! state->vertex_array_binding && ! client_has_vertex_attrib_array_set (CLIENT (client))) {
        caching_client_clear_attribute_list_data (CLIENT(client));
        return;
    }

    if (state->framebuffer_binding) {
        framebuffer_t *framebuffer = egl_state_lookup_cached_framebuffer (state, state->framebuffer_binding);
        if (framebuffer && framebuffer->id && framebuffer->complete == FRAMEBUFFER_INCOMPLETE) {
            caching_client_clear_attribute_list_data (CLIENT(client));
            caching_client_glSetError (client, GL_INVALID_FRAMEBUFFER_OPERATION);
            return;
        }
    }

    /* If we aren't actually passing any indices then do not execute anything. */
    bool copy_indices = !state->vertex_array_binding && !state->element_array_buffer_binding;
    size_t index_array_size = calculate_index_array_size (type, count);
    if ((! state->element_array_buffer_binding && ! indices) || (copy_indices && ! index_array_size)) {
        caching_client_clear_attribute_list_data (CLIENT(client));
        goto finish;
    }

    link_list_t *arrays_to_free = NULL;

    char* indices_to_pass = (char*) indices;
    command_gldrawelements_t *command = NULL;
    size_t array_size = 0;

    /* FIXME:  We do not handle where indices is in element_array_buffer
       while vertex attribs are not in array_buffer, because in
       this case, we could not compute the number of vertex attrib
       elements */
    if (copy_indices) {
        size_t elements_count = _get_elements_count (type, indices, count);
        caching_client_setup_vertex_attrib_pointer_if_necessary (
            CLIENT (client),
            elements_count, &arrays_to_free,
            (command_t **)&command,
            &array_size,
            index_array_size);

        if (command) {
            ((command_t *)command)->type = COMMAND_GLDRAWELEMENTS;
            ((command_t *)command)->size = command_get_size (COMMAND_GLDRAWELEMENTS) + array_size + index_array_size;
            ((command_t *)command)->token = 0;
            indices_to_pass = ((char *) command) + command_get_size (COMMAND_GLDRAWELEMENTS) + array_size;
        } else {
            indices_to_pass = malloc (index_array_size);
            link_list_prepend (&arrays_to_free, indices_to_pass, free);
        }
        memcpy (indices_to_pass, indices, index_array_size);
    }

    if (! command)
        command = (command_gldrawelements_t *) client_get_space_for_command (COMMAND_GLDRAWELEMENTS);

    command_gldrawelements_init (&command->header, mode, count, type, indices_to_pass);
    ((command_gldrawelements_t *) command)->arrays_to_free = arrays_to_free;
    client_run_command_async (&command->header);

finish:
    caching_client_clear_attribute_list_data (CLIENT(client));
    if (framebuffer && framebuffer->id && framebuffer->complete == FRAMEBUFFER_COMPLETE_UNKNOWN)
        caching_client_set_needs_get_error (CLIENT (client));
}

static void
caching_client_glFramebufferRenderbuffer (void* client, GLenum target, GLenum attachment,
                                          GLenum renderbuffertarget,
                                          GLenum renderbuffer)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (target != GL_FRAMEBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    } else if (renderbuffertarget != GL_RENDERBUFFER && renderbuffer != 0) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glFramebufferRenderbuffer (client, target, attachment, renderbuffertarget, renderbuffer);
    
    /* update framebuffer in cache */
    if (state->framebuffer_binding) {
        framebuffer_t *framebuffer = egl_state_lookup_cached_framebuffer (state, state->framebuffer_binding);
        if (framebuffer) {
            framebuffer->complete = FRAMEBUFFER_COMPLETE_UNKNOWN;
            
            if (attachment == GL_COLOR_ATTACHMENT0)
                framebuffer->attached_color_buffer = renderbuffer;
            else if (attachment == GL_DEPTH_ATTACHMENT)
                framebuffer->attached_depth_buffer = renderbuffer;
            else if (attachment == GL_STENCIL_ATTACHMENT)
                framebuffer->attached_stencil_buffer = renderbuffer;
        }
    }
    /* update renderbuffer cache */
    renderbuffer_t *rb = egl_state_lookup_cached_renderbuffer (state, renderbuffer);
    if (rb)
        rb->framebuffer_id = state->framebuffer_binding;
}

static void
caching_client_glFramebufferTexture2D (void* client,
                                       GLenum target,
                                       GLenum attachment,
                                       GLenum textarget,
                                       GLuint texture,
                                       GLint level)
{
    texture_t *tex; 
    GLuint framebuffer_id;

    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (target != GL_FRAMEBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glFramebufferTexture2D (client, target, attachment,
                                                                   textarget, texture, level);

    /* get cached texture */
    tex = egl_state_lookup_cached_texture (state, texture);
    if (tex) {
        tex->framebuffer_id = state->framebuffer_binding;
        framebuffer_id = tex->framebuffer_id;
        /* update framebuffer in cache */
        if (framebuffer_id) {
            framebuffer_t *framebuffer = egl_state_lookup_cached_framebuffer (state, framebuffer_id);
            if (framebuffer) {
                framebuffer->attached_image = texture;
                framebuffer->complete = FRAMEBUFFER_COMPLETE_UNKNOWN;
            }
        }
    }
}

static void
caching_client_glFrontFace (void* client, GLenum mode)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    if (state->front_face == mode)
        return;

    if (! (mode == GL_CCW || mode == GL_CW)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    state->front_face = mode;
    CACHING_CLIENT(client)->super_dispatch.glFrontFace (client, mode);
}

static void
caching_client_glGenBuffers (void* client, GLsizei n, GLuint *buffers)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state) {
        memset (buffers, 0, sizeof (GLuint) * n);
        return;
    }

    if (n < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    name_handler_alloc_names (state->buffer_name_handler, n, buffers);
    GLuint *server_buffers = (GLuint *)malloc (n * sizeof (GLuint));
    memcpy (server_buffers, buffers, n * sizeof (GLuint));

    CACHING_CLIENT(client)->super_dispatch.glGenBuffers (client, n, server_buffers);
}

static void
caching_client_glGenFramebuffers (void* client, GLsizei n, GLuint *framebuffers)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state) {
        memset (framebuffers, 0, sizeof (GLuint) * n);
        return;
    }

    if (n < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    name_handler_alloc_names (state->framebuffer_name_handler, n, framebuffers);
    GLuint *server_framebuffers = (GLuint *)malloc (n * sizeof (GLuint));
    memcpy (server_framebuffers, framebuffers, n * sizeof (GLuint));

    CACHING_CLIENT(client)->super_dispatch.glGenFramebuffers (client, n, server_framebuffers);
    
    /* add framebuffers to cache */
    int i;
    for (i = 0; i < n; i++)
        egl_state_create_cached_framebuffer (state, framebuffers[i]);
}

static void
caching_client_glGenRenderbuffers (void* client, GLsizei n, GLuint *renderbuffers)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state) {
        memset (renderbuffers, 0, sizeof (GLuint) * n);
        return;
    }

    if (n < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    name_handler_alloc_names (state->renderbuffer_name_handler, n, renderbuffers);
    GLuint *server_renderbuffers = (GLuint *)malloc (n * sizeof (GLuint));
    memcpy (server_renderbuffers, renderbuffers, n * sizeof (GLuint));

    CACHING_CLIENT(client)->super_dispatch.glGenRenderbuffers (client, n, server_renderbuffers);
    
    /* add renderbuffers to cache */
    int i;
    for (i = 0; i < n; i++)
        egl_state_create_cached_renderbuffer (state, renderbuffers[i]);
}

static void
caching_client_glGenTextures (void* client, GLsizei n, GLuint *textures)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state) {
        memset (textures, 0, sizeof (GLuint) * n);
        return;
    }

    if (n < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    name_handler_alloc_names (state->texture_name_handler, n, textures);

    GLuint *server_textures = (GLuint *)malloc (n * sizeof (GLuint));
    memcpy (server_textures, textures, n * sizeof (GLuint));

    CACHING_CLIENT(client)->super_dispatch.glGenTextures (client, n, server_textures);

    /* add textures to cache */
    int i;
    for (i = 0; i < n; i++)
        egl_state_create_cached_texture (state, textures[i]);
}

static void
caching_client_glGenerateMipmap (void* client, GLenum target)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (! is_valid_TextureBindTarget (target)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glGenerateMipmap (client, target);
}

static void caching_client_glGetActiveAttrib (void *client,
                                              GLuint program,
                                              GLuint index,
                                              GLsizei bufsize,
                                              GLsizei *length,
                                              GLint *size,
                                              GLenum *type,
                                              GLchar *name)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    CACHING_CLIENT(client)->super_dispatch.glGetActiveAttrib (client, program, index, bufsize,
                                                              length, size, type, name);

    if (*length == 0)
        caching_client_set_needs_get_error (CLIENT (client));
}

static void caching_client_glGetActiveUniform (void *client,
                                               GLuint program,
                                               GLuint index,
                                               GLsizei bufsize,
                                               GLsizei *length,
                                               GLint *size,
                                               GLenum *type,
                                               GLchar *name)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    CACHING_CLIENT(client)->super_dispatch.glGetActiveUniform (client, program, index, bufsize,
                                                               length, size, type, name);
    if (*length == 0)
        caching_client_set_needs_get_error (CLIENT (client));
}

static GLint
caching_client_glGetAttribLocation (void* client,
                                    GLuint program,
                                    const GLchar *name)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return 0;

    program_t *saved_program = egl_state_lookup_cached_program_err (client, program, GL_INVALID_OPERATION);
    if (!saved_program)
        return -1;

    GLuint *location = (GLuint *)hash_lookup (saved_program->attrib_location_cache,
                                              hash_str (name));
    if (location)
        return *location;

    GLuint result = CACHING_CLIENT(client)->super_dispatch.glGetAttribLocation (client, program, name);
    if (result == -1) {
        caching_client_set_needs_get_error (CLIENT (client));
        return -1;
    }

    GLuint *data = (GLuint *)malloc (sizeof (GLuint));
    *data = result;
    hash_insert (saved_program->attrib_location_cache, hash_str(name), data);
    return result;
}

static void
caching_client_glLinkProgram (void* client,
                              GLuint program)
{
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    program_t *saved_program = egl_state_lookup_cached_program_err (client, program, GL_INVALID_VALUE);
    if (!saved_program)
        return;

    CACHING_CLIENT(client)->super_dispatch.glLinkProgram (client, program);
}

static GLint
caching_client_glGetUniformLocation (void* client,
                                     GLuint program,
                                     const GLchar *name)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return 0;

    program_t *saved_program = egl_state_lookup_cached_program_err (client, program, GL_INVALID_VALUE);
    if (!saved_program)
        return -1;

    GLuint *location = (GLuint *)hash_lookup (saved_program->uniform_location_cache,
                                              hash_str (name));
    if (location)
        return *location;

    GLuint result = CACHING_CLIENT(client)->super_dispatch.glGetUniformLocation (client, program, name);
    if (result == -1) {
        caching_client_set_needs_get_error (CLIENT (client));
        return -1;
    }

    GLuint *data = (GLuint *)malloc (sizeof (GLuint));
    *data = result;
    hash_insert (saved_program->uniform_location_cache, hash_str(name), data);

    location_properties_t *location_properties = (location_properties_t *) malloc (sizeof (location_properties_t));
    location_properties->type = -1;
    hash_insert (saved_program->location_cache, *data, location_properties);
    return result;
}

static GLenum
caching_client_glGetError (void* client)
{
    GLenum error = GL_INVALID_OPERATION;

    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return GL_INVALID_OPERATION;
    if (! state->need_get_error) {
        error = state->error;
        state->error = GL_NO_ERROR;
        return error;
    }

    error = CACHING_CLIENT(client)->super_dispatch.glGetError (client);

    caching_client_reset_set_needs_get_error (CLIENT (client));
    state->error = GL_NO_ERROR;

    return error;
}

static void
caching_client_glGetFramebufferAttachmentParameteriv (void* client,
                                                      GLenum target,
                                                      GLenum attachment,
                                                      GLenum pname,
                                                      GLint *params)
{
    GLint original_params = params[0];

    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (target != GL_FRAMEBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glGetFramebufferAttachmentParameteriv (client, target,
                                                                                  attachment, pname,
                                                                                  params);
    if (original_params == params[0])
        caching_client_set_needs_get_error (CLIENT (client));
}

static void
caching_client_glGetRenderbufferParameteriv (void* client, GLenum target,
                                              GLenum pname,
                                              GLint *params)
{
    GLint original_params = params[0];

    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (target != GL_RENDERBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glGetRenderbufferParameteriv (client, target, pname, params);
    if (original_params == params[0])
        caching_client_set_needs_get_error (CLIENT (client));
}

static const GLubyte *
caching_client_glGetString (void* client, GLenum name)
{
    const GLubyte *result = NULL;
    int length = 0;

    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return NULL;

    if (! is_valid_StringType (name)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return NULL;
    }

    switch (name) {
    case GL_VENDOR:
        if (state->vendor_string)
            return (const GLubyte *)state->vendor_string;
        break;
    case GL_RENDERER:
        if (state->renderer_string)
            return (const GLubyte *)state->renderer_string;
        break;
    case GL_VERSION:
        if (state->version_string)
            return (const GLubyte *)state->version_string;
        break;
    case GL_SHADING_LANGUAGE_VERSION:
        if (state->shading_language_version_string)
            return (const GLubyte *)state->shading_language_version_string;
        break;
    case GL_EXTENSIONS:
        if (state->extensions_string)
            return (const GLubyte *)state->extensions_string;
        break;
    default:
        break;
    }

    result = CACHING_CLIENT(client)->super_dispatch.glGetString (client, name);

    if (result == 0)
        caching_client_set_needs_get_error (CLIENT (client));

    length = strlen ((char *)result);
    switch (name) {
    case GL_VENDOR:
        state->vendor_string = (char *)malloc (sizeof (char) * (length+1));
        memcpy (state->vendor_string, result, length);
        state->vendor_string[length] = 0;
        break;
    case GL_RENDERER:
        state->renderer_string = (char *)malloc (sizeof (char) * (length+1));
        memcpy (state->renderer_string, result, length);
        state->renderer_string[length] = 0;
        break;
    case GL_VERSION:
        state->version_string = (char *)malloc (sizeof (char) * (length+1));
        memcpy (state->version_string, result, length);
        state->version_string[length] = 0;
        break;
    case GL_SHADING_LANGUAGE_VERSION:
        state->shading_language_version_string = (char *)malloc (sizeof (char) * (length+1));
        memcpy (state->shading_language_version_string, result, length);
        state->shading_language_version_string[length] = 0;
        break;
    case GL_EXTENSIONS:
        state->extensions_string = (char *)malloc (sizeof (char) * (length+1));
        memcpy (state->extensions_string, result, length);
        state->extensions_string[length] = 0;

        state->supports_element_index_uint = strstr (state->extensions_string, "GL_OES_element_index_uint") ? true : false;
        state->supports_bgra = strstr (state->extensions_string, "GL_EXT_texture_format_BGRA8888") ? true : false;
        break;
    default:
        break;
    }

    return result;
}

static texture_t*
caching_client_lookup_cached_texture (egl_state_t *state, GLenum target)
{
    GLuint tex_id;
    texture_t *texture;

    if (target == GL_TEXTURE_2D)
        tex_id = state->texture_binding[0];
    else if (target == GL_TEXTURE_CUBE_MAP)
        tex_id = state->texture_binding[1];
    else
        tex_id = state->texture_binding_3d;

    texture = egl_state_lookup_cached_texture (state, tex_id);

    return texture;
}

static void
caching_client_glGetTexParameteriv (void* client, GLenum target, GLenum pname,
                                     GLint *params)
{
    texture_t *texture;

    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (! is_valid_GetTexParamTarget (target)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    if (! is_valid_TextureParameter (pname)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    texture = caching_client_lookup_cached_texture (state, target);
    if (! texture)
        texture = caching_client_get_default_texture ();

    if (pname == GL_TEXTURE_MAG_FILTER)
        *params = texture->texture_mag_filter;
    else if (pname == GL_TEXTURE_MIN_FILTER)
        *params = texture->texture_min_filter;
    else if (pname == GL_TEXTURE_WRAP_S)
        *params = texture->texture_wrap_s;
    else if (pname == GL_TEXTURE_WRAP_T)
        *params = texture->texture_wrap_t;
    else if (pname == GL_TEXTURE_WRAP_R_OES)
        *params = texture->texture_3d_wrap_r;
    else if (pname == GL_TEXTURE_MAX_ANISOTROPY_EXT)
        *params = texture->texture_max_anisotropy;
}

static void
caching_client_glGetTexParameterfv (void* client, GLenum target, GLenum pname, GLfloat *params)
{
    GLint paramsi;
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    /* XXX: GL_TEXTURE_MAX_ANISOTROPY_EXT returns a float,
     * we can not use caching_client_glGetTexParameteriv
     */

    if (pname == GL_TEXTURE_MAX_ANISOTROPY_EXT) {
        texture_t *texture;
        if (! is_valid_GetTexParamTarget (target)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        if (! is_valid_TextureParameter (pname)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        texture = caching_client_lookup_cached_texture (state, target);
        if (! texture)
            texture = caching_client_get_default_texture ();

        *params = texture->texture_max_anisotropy;
        return;
    }

    caching_client_glGetTexParameteriv (client, target, pname, &paramsi);
    *params = paramsi;
}

static void
caching_client_glGetUniformfv (void* client, GLuint program,
                               GLint location,  GLfloat *params)
{
    GLfloat original_params = *params;

    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    command_t *command = client_get_space_for_command (COMMAND_GLGETUNIFORMFV);
    command_glgetuniformfv_init (command, program, location, params);
    client_run_command (command);

    if (original_params == *params)
        caching_client_set_needs_get_error (CLIENT (client));
}

static void
caching_client_glGetUniformiv (void* client, GLuint program,
                               GLint location,  GLint *params)
{
    GLint original_params = *params;

    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    CACHING_CLIENT(client)->super_dispatch.glGetUniformiv (client, program, location, params);

    if (original_params == *params)
        caching_client_set_needs_get_error (CLIENT (client));
}

static void
caching_client_glGetVertexAttribfv (void* client, GLuint index, GLenum pname,
                                     GLfloat *params)
{
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i;

    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    attrib_list = &state->vertex_attribs;
    attribs = attrib_list->attribs;
    count = attrib_list->count;

    if (! is_valid_VertexAttribute (pname)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    /* check index is too large */
    if (caching_client_does_index_overflow (client, index))
        return;

    /* we cannot use client state */
    if (state->vertex_array_binding) {
        caching_client_set_needs_get_error (CLIENT (client));
        CACHING_CLIENT(client)->super_dispatch.glGetVertexAttribfv (client, index, pname, params);
        return;
    }

    /* look into client state */
    for (i = 0; i < count; i++) {
        if (attribs[i].index == index) {
            switch (pname) {
            case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
                *params = attribs[i].array_buffer_binding;
                break;
            case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
                *params = attribs[i].array_enabled;
                break;
            case GL_VERTEX_ATTRIB_ARRAY_SIZE:
                *params = attribs[i].size;
                break;
            case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
                *params = attribs[i].stride;
                break;
            case GL_VERTEX_ATTRIB_ARRAY_TYPE:
                *params = attribs[i].type;
                break;
            case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
                *params = attribs[i].array_normalized;
                break;
            default:
                memcpy (params, attribs[i].current_attrib, sizeof (GLfloat) * 4);
                break;
            }
            return;
        }
    }

    /* we did not find anyting, return default */
    switch (pname) {
    case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
        *params = state->array_buffer_binding;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
        *params = false;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_SIZE:
        *params = 4;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
        *params = 0;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_TYPE:
        *params = GL_FLOAT;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
        *params = GL_FALSE;
        break;
    default:
        memset (params, 0, sizeof (GLfloat) * 4);
        break;
    }
}

static void
caching_client_glGetVertexAttribiv (void* client, GLuint index, GLenum pname, GLint *params)
{
    GLfloat paramsf[4];
    int i;

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    caching_client_glGetVertexAttribfv (client, index, pname, paramsf);

    if (pname == GL_CURRENT_VERTEX_ATTRIB) {
        for (i = 0; i < 4; i++)
            params[i] = paramsf[i];
    } else
        *params = paramsf[0];
}

static void
caching_client_glGetVertexAttribPointerv (void* client, GLuint index, GLenum pname,
                                            GLvoid **pointer)
{
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i;

    INSTRUMENT();

    *pointer = 0;

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    attrib_list = &state->vertex_attribs;
    attribs = attrib_list->attribs;
    count = attrib_list->count;

    if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    /* XXX: check index validity */
    if (caching_client_does_index_overflow (client, index))
        return;

    /* we cannot use client state */
    if (state->vertex_array_binding) {
        CACHING_CLIENT(client)->super_dispatch.glGetVertexAttribPointerv (client, index, pname, pointer);
        if (*pointer == NULL)
            caching_client_set_needs_get_error (CLIENT (client));
        return;
    }

    /* look into client state */
    for (i = 0; i < count; i++) {
        if (attribs[i].index == index) {
            *pointer = attribs[i].pointer;
            return;
        }
    }
}

static void
caching_client_glHint (void* client, GLenum target, GLenum mode)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    if (target == GL_GENERATE_MIPMAP_HINT && state->generate_mipmap_hint == mode)
        return;

    if (! is_valid_HintMode (mode)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    if (target == GL_GENERATE_MIPMAP_HINT)
        state->generate_mipmap_hint = mode;

    CACHING_CLIENT(client)->super_dispatch.glHint (client, target, mode);

    if (target != GL_GENERATE_MIPMAP_HINT)
        caching_client_set_needs_get_error (CLIENT (client));
}

static GLboolean
caching_client_glIsEnabled (void* client, GLenum cap)
{
    GLboolean result = GL_FALSE;

    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return result;
    switch (cap) {
    case GL_BLEND:
        result = state->blend;
        break;
    case GL_CULL_FACE:
        result = state->cull_face;
        break;
    case GL_DEPTH_TEST:
        result = state->depth_test;
        break;
    case GL_DITHER:
        result = state->dither;
        break;
    case GL_POLYGON_OFFSET_FILL:
        result = state->polygon_offset_fill;
        break;
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
        result = state->sample_alpha_to_coverage;
        break;
    case GL_SAMPLE_COVERAGE:
        result = state->sample_coverage;
        break;
    case GL_SCISSOR_TEST:
        result = state->scissor_test;
        break;
    case GL_STENCIL_TEST:
        result = state->stencil_test;
        break;
    default:
        caching_client_glSetError (client, GL_INVALID_ENUM);
        break;
    }
    return result;
}

static void
caching_client_glLineWidth (void* client, GLfloat width)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    if (state->line_width == width)
        return;

    if (width < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    state->line_width = width;
    CACHING_CLIENT(client)->super_dispatch.glLineWidth (client, width);
}

static GLboolean
caching_client_glIsTexture (void *client, GLuint texture)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return GL_FALSE;
    return egl_state_lookup_cached_texture (state, texture) ? GL_TRUE : GL_FALSE;
}

static void
caching_client_glPixelStorei (void* client, GLenum pname, GLint param)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if ((pname == GL_PACK_ALIGNMENT && state->pack_alignment == param) ||
        (pname == GL_UNPACK_ALIGNMENT && state->unpack_alignment == param) ||
        (pname == GL_UNPACK_ROW_LENGTH && state->unpack_row_length == param) ||
        (pname == GL_UNPACK_SKIP_ROWS && state->unpack_skip_rows == param) ||
        (pname == GL_UNPACK_SKIP_PIXELS && state->unpack_skip_pixels == param))
        return;

    if (! is_valid_PixelStoreAlignment(param) &&
        (pname == GL_PACK_ALIGNMENT || pname == GL_UNPACK_ALIGNMENT)) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    } else if (param < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    switch (pname) {
    case GL_PACK_ALIGNMENT:
       state->pack_alignment = param;
       break;
    case GL_UNPACK_ALIGNMENT:
       state->unpack_alignment = param;
       break;
    case GL_UNPACK_ROW_LENGTH:
       state->unpack_row_length = param;
       return;
    case GL_UNPACK_SKIP_ROWS:
       state->unpack_skip_rows = param;
       return;
    case GL_UNPACK_SKIP_PIXELS:
       state->unpack_skip_pixels = param;
       return;
    default:
       caching_client_glSetError (client, GL_INVALID_VALUE);
       return;
    }

    CACHING_CLIENT(client)->super_dispatch.glPixelStorei (client, pname, param);
}

static void
caching_client_glPolygonOffset (void* client, GLfloat factor, GLfloat units)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    if (state->polygon_offset_factor == factor &&
        state->polygon_offset_units == units)
        return;

    state->polygon_offset_factor = factor;
    state->polygon_offset_units = units;

    CACHING_CLIENT(client)->super_dispatch.glPolygonOffset (client, factor, units);
}

static void
caching_client_glSampleCoverage (void* client, GLclampf value, GLboolean invert)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    if (value == state->sample_coverage_value &&
        invert == state->sample_coverage_invert)
        return;

    state->sample_coverage_invert = invert;
    state->sample_coverage_value = value;

    CACHING_CLIENT(client)->super_dispatch.glSampleCoverage (client, value, invert);
}

static void
caching_client_glScissor (void* client, GLint x, GLint y, GLsizei width, GLsizei height)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    if (x == state->scissor_box[0]     &&
        y == state->scissor_box[1]     &&
        width == state->scissor_box[2] &&
        height == state->scissor_box[3])
        return;

    if (width < 0 || height < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    state->scissor_box[0] = x;
    state->scissor_box[1] = y;
    state->scissor_box[2] = width;
    state->scissor_box[3] = height;

    CACHING_CLIENT(client)->super_dispatch.glScissor (client, x, y, width, height);
}

static void
caching_client_glStencilFuncSeparate (void* client, GLenum face, GLenum func,
                                       GLint ref, GLuint mask)
{
    bool needs_call = false;

    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    if (! is_valid_FaceType (face)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    if (! is_valid_CmpFunction (func)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    switch (face) {
    case GL_FRONT:
        if (func != state->stencil_func ||
            ref != state->stencil_ref   ||
            mask != state->stencil_value_mask) {
            state->stencil_func = func;
            state->stencil_ref = ref;
            state->stencil_value_mask = mask;
            needs_call = true;
        }
        break;
    case GL_BACK:
        if (func != state->stencil_back_func ||
            ref != state->stencil_back_ref   ||
            mask != state->stencil_back_value_mask) {
            state->stencil_back_func = func;
            state->stencil_back_ref = ref;
            state->stencil_back_value_mask = mask;
            needs_call = true;
        }
        break;
    default:
        if (func != state->stencil_back_func       ||
            func != state->stencil_func            ||
            ref != state->stencil_back_ref         ||
            ref != state->stencil_ref              ||
            mask != state->stencil_back_value_mask ||
            mask != state->stencil_value_mask) {
            state->stencil_back_func = func;
            state->stencil_func = func;
            state->stencil_back_ref = ref;
            state->stencil_ref = ref;
            state->stencil_back_value_mask = mask;
            state->stencil_value_mask = mask;
            needs_call = true;
        }
        break;
    }

    if (needs_call)
        CACHING_CLIENT(client)->super_dispatch.glStencilFuncSeparate (client, face, func, ref, mask);
}

static void
caching_client_glStencilFunc (void* client, GLenum func, GLint ref, GLuint mask)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    caching_client_glStencilFuncSeparate (client, GL_FRONT_AND_BACK, func, ref, mask);
}

static void
caching_client_glStencilMaskSeparate (void* client, GLenum face, GLuint mask)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (! is_valid_FaceType (face)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    bool needs_call = false;
    switch (face) {
    case GL_FRONT:
        if (mask != state->stencil_writemask) {
            state->stencil_writemask = mask;
            needs_call = true;
        }
        break;
    case GL_BACK:
        if (mask != state->stencil_back_writemask) {
            state->stencil_back_writemask = mask;
            needs_call = true;
        }
        break;
    default:
        if (mask != state->stencil_back_writemask ||
            mask != state->stencil_writemask) {
            state->stencil_back_writemask = mask;
            state->stencil_writemask = mask;
            needs_call = true;
        }
        break;
    }

    if (needs_call)
        CACHING_CLIENT(client)->super_dispatch.glStencilMaskSeparate (client, face, mask);
}

static void
caching_client_glStencilMask (void* client, GLuint mask)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    caching_client_glStencilMaskSeparate (client, GL_FRONT_AND_BACK, mask);
}

static void
caching_client_glStencilOpSeparate (void* client, GLenum face, GLenum sfail,
                                     GLenum dpfail, GLenum dppass)
{
    bool needs_call = false;

    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (! is_valid_FaceType (face) ||
        ! is_valid_StencilOp (sfail) ||
        ! is_valid_StencilOp (dpfail) ||
        ! is_valid_StencilOp (dppass)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    switch (face) {
    case GL_FRONT:
        if (sfail != state->stencil_fail             ||
            dpfail != state->stencil_pass_depth_fail ||
            dppass != state->stencil_pass_depth_pass) {
            state->stencil_fail = sfail;
            state->stencil_pass_depth_fail = dpfail;
            state->stencil_pass_depth_pass = dppass;
            needs_call = true;
        }
        break;
    case GL_BACK:
        if (sfail != state->stencil_back_fail             ||
            dpfail != state->stencil_back_pass_depth_fail ||
            dppass != state->stencil_back_pass_depth_pass) {
            state->stencil_back_fail = sfail;
            state->stencil_back_pass_depth_fail = dpfail;
            state->stencil_back_pass_depth_pass = dppass;
            needs_call = true;
        }
        break;
    default:
        if (sfail != state->stencil_fail                  ||
            dpfail != state->stencil_pass_depth_fail      ||
            dppass != state->stencil_pass_depth_pass      ||
            sfail != state->stencil_back_fail             ||
            dpfail != state->stencil_back_pass_depth_fail ||
            dppass != state->stencil_back_pass_depth_pass) {
            state->stencil_fail = sfail;
            state->stencil_pass_depth_fail = dpfail;
            state->stencil_pass_depth_pass = dppass;
            state->stencil_back_fail = sfail;
            state->stencil_back_pass_depth_fail = dpfail;
            state->stencil_back_pass_depth_pass = dppass;
            needs_call = true;
        }
        break;
    }

    if (needs_call)
        CACHING_CLIENT(client)->super_dispatch.glStencilOpSeparate (client, face, sfail, dpfail, dppass);
}

static void
caching_client_glStencilOp (void* client, GLenum sfail, GLenum dpfail, GLenum dppass)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    caching_client_glStencilOpSeparate (client, GL_FRONT_AND_BACK, sfail, dpfail, dppass);
}

static void
caching_client_glTexParameteri (void* client, GLenum target, GLenum pname, GLint param)
{
    INSTRUMENT();
    texture_t *texture = NULL;
    bool needs_call = false;

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (! is_valid_GetTexParamTarget (target)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    if (! is_valid_TextureParameter (pname)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    texture = caching_client_lookup_cached_texture (state, target);

    if ((pname == GL_TEXTURE_MAG_FILTER && ! is_valid_TextureMagFilterMode (param)) ||
        (pname == GL_TEXTURE_MIN_FILTER && ! is_valid_TextureMinFilterMode (param)) ||
        ((pname == GL_TEXTURE_WRAP_S || pname == GL_TEXTURE_WRAP_T || pname == GL_TEXTURE_WRAP_R_OES) &&
            ! is_valid_TextureWrapMode (param))) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    if ((pname == GL_TEXTURE_MAX_ANISOTROPY_EXT) && param < 1) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    if (! texture)
        texture = caching_client_get_default_texture ();

    if (pname == GL_TEXTURE_MAG_FILTER &&
        texture->texture_mag_filter != param) {
        texture->texture_mag_filter = param;
        needs_call = true;
    }
    else if (pname == GL_TEXTURE_MIN_FILTER &&
             texture->texture_min_filter != param) {
        texture->texture_min_filter = param;
        needs_call = true;
    }
    else if (pname == GL_TEXTURE_WRAP_S &&
             texture->texture_wrap_s != param) {
        texture->texture_wrap_s = param;
        needs_call = true;
    }
    else if (pname == GL_TEXTURE_WRAP_T &&
             texture->texture_wrap_t != param) {
        texture->texture_wrap_t = param;
        needs_call = true;
    }
    else if (pname == GL_TEXTURE_WRAP_R_OES &&
        texture->texture_3d_wrap_r != param) {
        texture->texture_3d_wrap_r = param;
        needs_call = true;
    }
    else if (pname == GL_TEXTURE_MAX_ANISOTROPY_EXT &&
             texture->texture_max_anisotropy != param) {
        texture->texture_max_anisotropy = param;
        needs_call = true;
    }

    if (needs_call)
        CACHING_CLIENT(client)->super_dispatch.glTexParameteri (client, target, pname, param);
}

static void
caching_client_glTexParameterf (void* client, GLenum target, GLenum pname, GLfloat param)
{
    GLint parami = param;

    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    /* XXX: GL_TEXTURE_MAX_ANISOTROPY_EXT sets a float,
     * we can not use caching_client_glTexParameteri
     */

    if (pname == GL_TEXTURE_MAX_ANISOTROPY_EXT) {
        texture_t *texture;
        bool needs_call = false;

        if (! is_valid_GetTexParamTarget (target)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        if (! is_valid_TextureParameter (pname)) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }

        if (param < 1.0) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }

        texture = caching_client_lookup_cached_texture (state, target);
        if (! texture)
            texture = caching_client_get_default_texture ();

        if (texture->texture_max_anisotropy != param) {
            texture->texture_max_anisotropy = param;
            needs_call = true;
        }

        if (needs_call)
            CACHING_CLIENT(client)->super_dispatch.glTexParameterf (client, target, pname, param);
    }

    caching_client_glTexParameteri (client, target, pname, parami);
}

static void
caching_client_glTexImage2D (void* client, GLenum target, GLint level,
                             GLint internalformat, GLsizei width,
                             GLsizei height, GLint border, GLenum format,
                             GLenum type, const void *pixels)
{
    GLuint tex_id;

    egl_state_t *state = client_get_current_state (CLIENT (client));

    INSTRUMENT();

    if (! state)
        return;

    if (level < 0 || width < 0 || height < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    if (internalformat != format) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return;
    }

    if (! is_valid_TextureTarget (target)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    if (type == GL_UNSIGNED_SHORT_5_6_5 && format != GL_RGB) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return;
    }

    /* check we support GL_EXT_texture_format_BGRA888 */
    if (! state->extensions_string) {
	caching_client_glGetString (client, GL_EXTENSIONS);
    }

    if ((type == GL_UNSIGNED_SHORT_4_4_4_4 ||
         type == GL_UNSIGNED_SHORT_5_5_5_1) &&
         format != GL_RGBA) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return;
    }

    if (! state->supports_bgra && format == GL_BGRA_EXT) {
	caching_client_glSetError (client, GL_INVALID_OPERATION);
	return;
    }

    /* FIXME: we need more checks on max width/height and level */
    if (target == GL_TEXTURE_2D)
        tex_id = state->texture_binding[0];
    else
        tex_id = state->texture_binding[1];

    texture_t *texture = egl_state_lookup_cached_texture (state, tex_id);
    if (! texture)
        caching_client_set_needs_get_error (CLIENT (client));
    else {
        texture->internal_format = internalformat;
        texture->width = width;
        texture->height = height;
        texture->data_type = type;
    }

    CACHING_CLIENT(client)->super_dispatch.glTexImage2D (client, target, level, internalformat,
                                                         width, height, border, format, type, pixels);
    
    /* update framebuffer in cache */
    if (texture && texture->framebuffer_id) {
        framebuffer_t *framebuffer = egl_state_lookup_cached_framebuffer (state, texture->framebuffer_id);
        if (framebuffer)
            framebuffer->complete = FRAMEBUFFER_COMPLETE_UNKNOWN;
    }
}

static void
caching_client_glTexSubImage2D (void* client,
                                GLenum target,
                                GLint level,
                                GLint xoffset,
                                GLint yoffset,
                                GLsizei width,
                                GLsizei height,
                                GLenum format,
                                GLenum type,
                                const void *pixels)
{
    GLuint tex_id;
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    INSTRUMENT();

    if (level < 0 || width < 0 || height < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    if (! (target == GL_TEXTURE_2D                  ||
           target == GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
           target == GL_TEXTURE_CUBE_MAP_NEGATIVE_X ||
           target == GL_TEXTURE_CUBE_MAP_POSITIVE_Y ||
           target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y ||
           target == GL_TEXTURE_CUBE_MAP_POSITIVE_Z ||
           target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    if (type == GL_UNSIGNED_SHORT_5_6_5 && format != GL_RGB) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return;
    }

    if ((type == GL_UNSIGNED_SHORT_4_4_4_4 ||
         type == GL_UNSIGNED_SHORT_5_5_5_1) &&
         format != GL_RGBA) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return;
    }

    if (! is_valid_TextureFormat (format)) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return;
    }

    if (target == GL_TEXTURE_2D)
        tex_id = state->texture_binding[0];
    else
        tex_id = state->texture_binding[1];

    texture_t *texture = egl_state_lookup_cached_texture (state, tex_id);
    if (! texture) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return;
    } else {
        if (xoffset + width > texture->width || yoffset + height > texture->height) {
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }
    }

    /* if internal format does not match format, we need to set
       needs_get_error = true
     */
    if (texture->internal_format != format)
        caching_client_set_needs_get_error (CLIENT (client));

    /* FIXME: we need to check level */

    CACHING_CLIENT(client)->super_dispatch.glTexSubImage2D (client, target, level, xoffset, yoffset,
                                                            width, height, format, type, pixels);
}

static location_properties_t *
_synthesize_uniform_error(void *client,
                          GLint location,
                          GLenum program_error)
{
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return 0;

    program_t *saved_program = egl_state_lookup_cached_program_err (client,
                                                                    state->current_program,
                                                                    GL_INVALID_OPERATION);
    if (!saved_program)
        return 0;

    location_properties_t *location_properties = hash_lookup(saved_program->location_cache, location);
    if (! location_properties) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return 0;
    }

    return location_properties;
}

static void
_location_has_valid_type(void *client, location_properties_t *location_properties, GLenum location_type)
{
    if (location_properties->type == -1) {
        GLenum error = CACHING_CLIENT(client)->super_dispatch.glGetError (client);

        if (error != GL_NO_ERROR)
            caching_client_glSetError (client, GL_INVALID_OPERATION);
        else
            location_properties->type = location_type;
    } else if (location_properties->type != location_type)
        caching_client_glSetError (client, GL_INVALID_OPERATION);
}

static void
caching_client_glUniform1f (void *client, GLint location, GLfloat v0)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_error (client,
                                                                            location,
                                                                            GL_INVALID_OPERATION);
    if (! location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniform1f (client, location, v0);

    _location_has_valid_type (client, location_properties, GL_FLOAT);
}

static void
caching_client_glUniform2f (void *client,
                             GLint location, 
                             GLfloat v0,
                             GLfloat v1)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_error (client,
                                                                            location,
                                                                            GL_INVALID_OPERATION);
    if (! location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniform2f (client, location, v0, v1);

    _location_has_valid_type (client, location_properties, GL_FLOAT_VEC2);
}

static void
caching_client_glUniform3f (void *client,
                             GLint location, 
                             GLfloat v0,
                             GLfloat v1,
                             GLfloat v2)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_error (client,
                                                                            location,
                                                                            GL_INVALID_OPERATION);
    if (! location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniform3f (client, location, v0, v1, v2);

    _location_has_valid_type (client, location_properties, GL_FLOAT_VEC3);
}

static void
caching_client_glUniform4f (void *client,
                             GLint location, 
                             GLfloat v0,
                             GLfloat v1,
                             GLfloat v2,
                             GLfloat v3)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_error (client,
                                                                            location,
                                                                            GL_INVALID_OPERATION);
    if (! location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniform4f (client, location, v0, v1, v2, v3);

    _location_has_valid_type (client, location_properties, GL_FLOAT_VEC4);
}

static void
caching_client_glUniform1i (void *client, GLint location, GLint v0)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_error (client,
                                                                            location,
                                                                            GL_INVALID_OPERATION);
    if (! location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniform1i (client, location, v0);

    _location_has_valid_type (client, location_properties, GL_INT);
}

static void
caching_client_glUniform2i (void *client,
                             GLint location, 
                             GLint v0,
                             GLint v1)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_error (client,
                                                                            location,
                                                                            GL_INVALID_OPERATION);
    if (! location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniform2i (client, location, v0, v1);

    _location_has_valid_type (client, location_properties, GL_INT_VEC2);
}

static void
caching_client_glUniform3i (void *client,
                             GLint location, 
                             GLint v0,
                             GLint v1,
                             GLint v2)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_error (client,
                                                                            location,
                                                                            GL_INVALID_OPERATION);
    if (! location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniform3i (client, location, v0, v1, v2);

    _location_has_valid_type (client, location_properties, GL_INT_VEC3);
}

static void
caching_client_glUniform4i (void *client,
                             GLint location, 
                             GLint v0,
                             GLint v1,
                             GLint v2,
                             GLint v3)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    location_properties_t *location_properties = _synthesize_uniform_error (client,
                                                                            location,
                                                                            GL_INVALID_OPERATION);
    if (! location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniform4i (client, location, v0, v1, v2, v3);

    _location_has_valid_type (client, location_properties, GL_INT_VEC4);
}

location_properties_t *
_synthesize_uniform_vector_error(void *client,
                                 GLint location,
                                 GLsizei count,
                                 GLenum program_error)
{
    if (count < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return false;
    }

    return _synthesize_uniform_error (client,
                                      location,
                                      GL_INVALID_OPERATION);
}

static void
caching_client_glUniform1fv (void *client,
                             GLint location,
                             GLsizei count,
                             const GLfloat *value)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_vector_error (client,
                                                                                   location,
                                                                                   count,
                                                                                   GL_INVALID_OPERATION);
    if (!location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniform1fv (client, location, count, value);

    _location_has_valid_type (client, location_properties, GL_FLOAT);
}

static void
caching_client_glUniform2fv (void *client,
                             GLint location,
                             GLsizei count,
                             const GLfloat *value)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_vector_error (client,
                                                                                   location,
                                                                                   count,
                                                                                   GL_INVALID_OPERATION);
    if (!location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniform2fv (client, location, count, value);

    _location_has_valid_type (client, location_properties, GL_FLOAT_VEC2);
}

static void
caching_client_glUniform3fv (void *client,
                             GLint location,
                             GLsizei count,
                             const GLfloat *value)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_vector_error (client,
                                                                                   location,
                                                                                   count,
                                                                                   GL_INVALID_OPERATION);
    if (!location_properties)
        return;


    CACHING_CLIENT(client)->super_dispatch.glUniform3fv (client, location, count, value);

    _location_has_valid_type (client, location_properties, GL_FLOAT_VEC3);
}

static void
caching_client_glUniform4fv (void *client,
                             GLint location,
                             GLsizei count,
                             const GLfloat *value)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_vector_error (client,
                                                                                   location,
                                                                                   count,
                                                                                   GL_INVALID_OPERATION);
    if (!location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniform4fv (client, location, count, value);

    _location_has_valid_type (client, location_properties, GL_FLOAT_VEC4);
}

static void
caching_client_glUniform1iv (void *client,
                             GLint location,
                             GLsizei count,
                             const GLint *value)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_vector_error (client,
                                                                                   location,
                                                                                   count,
                                                                                   GL_INVALID_OPERATION);
    if (!location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniform1iv (client, location, count, value);

    _location_has_valid_type (client, location_properties, GL_INT);
}

static void
caching_client_glUniform2iv (void *client,
                             GLint location,
                             GLsizei count,
                             const GLint *value)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_vector_error (client,
                                                                                   location,
                                                                                   count,
                                                                                   GL_INVALID_OPERATION);
    if (!location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniform2iv (client, location, count, value);

    _location_has_valid_type (client, location_properties, GL_INT_VEC2);
}

static void
caching_client_glUniform3iv (void *client,
                             GLint location,
                             GLsizei count,
                             const GLint *value)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_vector_error (client,
                                                                                   location,
                                                                                   count,
                                                                                   GL_INVALID_OPERATION);
    if (!location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniform3iv (client, location, count, value);

    _location_has_valid_type (client, location_properties, GL_INT_VEC3);
}

static void
caching_client_glUniform4iv (void *client,
                             GLint location,
                             GLsizei count,
                             const GLint *value)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_vector_error (client,
                                                                                   location,
                                                                                   count,
                                                                                   GL_INVALID_OPERATION);
    if (!location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniform4iv (client, location, count, value);

    _location_has_valid_type (client, location_properties, GL_INT_VEC4);
}

static void
caching_client_glUniformMatrix2fv (void* client,
                                   GLint location,
                                   GLsizei count,
                                   GLboolean transpose,
                                   const GLfloat *value)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_vector_error (client,
                                                                                   location,
                                                                                   count,
                                                                                   GL_INVALID_OPERATION);
    if (!location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniformMatrix2fv (client, location, count, transpose, value);

    _location_has_valid_type (client, location_properties, GL_FLOAT_MAT2);
}

static void
caching_client_glUniformMatrix3fv (void* client,
                                   GLint location,
                                   GLsizei count,
                                   GLboolean transpose,
                                   const GLfloat *value)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_vector_error (client,
                                                                                   location,
                                                                                   count,
                                                                                   GL_INVALID_OPERATION);
    if (!location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniformMatrix3fv (client, location, count, transpose, value);

    _location_has_valid_type (client, location_properties, GL_FLOAT_MAT3);
}

static void
caching_client_glUniformMatrix4fv (void* client,
                                   GLint location,
                                   GLsizei count,
                                   GLboolean transpose,
                                   const GLfloat *value)
{
    INSTRUMENT();
    location_properties_t *location_properties = _synthesize_uniform_vector_error (client,
                                                                                   location,
                                                                                   count,
                                                                                   GL_INVALID_OPERATION);
    if (!location_properties)
        return;

    CACHING_CLIENT(client)->super_dispatch.glUniformMatrix4fv (client, location, count, transpose, value);

    _location_has_valid_type (client, location_properties, GL_FLOAT_MAT4);
}

static void
caching_client_glUseProgram (void* client,
                             GLuint program_id)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    if (state->current_program == program_id)
        return;
    if (program_id < 0) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return;
    }

    /* From glUseProgram doc: if program is zero, then the current
     * rendering state refers to an invalid program object and the
     * results of shader execution are undefined. However, this is not
     * an error.*/
    if (program_id) {
        /* this maybe not right because this program may be invalid
         * object, we save here to save time in glGetError() */
        program_t *new_program = egl_state_lookup_cached_program_err (client, program_id, GL_INVALID_VALUE);
        if (!new_program)
            return;
    }

    CACHING_CLIENT(client)->super_dispatch.glUseProgram (client, program_id);

    /* this maybe not right because this program may be invalid
     * object, we save here to save time in glGetError() */
    program_t *current_program = (program_t *) egl_state_lookup_cached_shader_object (state, state->current_program);
    if (current_program && current_program->mark_for_deletion)
        egl_state_destroy_cached_shader_object (state, &current_program->base);
    state->current_program = program_id;
}

static void
caching_client_set_current_vertex_attrib (void* client, GLuint index, void *curr_attrib)
{
    GLfloat *current_attrib = curr_attrib;
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int i, found_index = -1;
    int count;
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (caching_client_does_index_overflow (client, index))
        return;

    attrib_list = &state->vertex_attribs;
    attribs = attrib_list->attribs;
    count = attrib_list->count;

    /* check existing client state */
    for (i = 0; i < count; i++) {
        if (attribs[i].index == index)
	    found_index = i;
    }

    if (found_index != -1) {
	memcpy (attribs[found_index].current_attrib, current_attrib, 4 * sizeof (GLfloat));
        return;
    }

    /* we have not found index */
    if (i < NUM_EMBEDDED) {
	memcpy (attribs[i].current_attrib, current_attrib, 4 * sizeof (GLfloat));
        attrib_list->count ++;
    } else {
        vertex_attrib_t *new_attribs = (vertex_attrib_t *)malloc (sizeof (vertex_attrib_t) * (count + 1));

        memcpy (new_attribs, attribs, (count+1) * sizeof (vertex_attrib_t));
        if (attribs != attrib_list->embedded_attribs)
            free (attribs);

	memcpy (attribs[i].current_attrib, current_attrib, 4 * sizeof (GLfloat));

        attrib_list->attribs = new_attribs;
        attrib_list->count ++;
    }
}


static void
caching_client_glVertexAttrib1f (void* client, GLuint index, GLfloat v0)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    GLfloat v[4] = {v0, 0, 0, 0};
    if (! state)
        return;

    caching_client_set_current_vertex_attrib (client, index, &v);
}

static void
caching_client_glVertexAttrib2f (void* client, GLuint index, GLfloat v0, GLfloat v1)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    GLfloat v[4] = {v0, v1, 0, 0};
    if (! state)
        return;

    caching_client_set_current_vertex_attrib (client, index, &v);
}

static void
caching_client_glVertexAttrib3f (void* client, GLuint index, GLfloat v0,
                                 GLfloat v1, GLfloat v2)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    GLfloat v[4] = {v0, v1, v2, 0};
    if (! state)
        return;

    caching_client_set_current_vertex_attrib (client, index, &v);
}

static void
caching_client_glVertexAttrib4f (void* client, GLuint index, GLfloat v0, GLfloat v1,
                                 GLfloat v2, GLfloat v3)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    GLfloat v[4] = {v0, v1, v2, v3};
    if (! state)
        return;

    caching_client_set_current_vertex_attrib (client, index, &v);
}

static void
caching_client_glVertexAttrib1fv (void* client, GLuint index, const GLfloat *v)
{
    INSTRUMENT();
    caching_client_glVertexAttrib1f(client, index, v[0]);
}

static void
caching_client_glVertexAttrib2fv (void* client, GLuint index, const GLfloat *v)
{
    INSTRUMENT();
    caching_client_glVertexAttrib2f(client, index, v[0], v[1]);

}

static void
caching_client_glVertexAttrib3fv (void* client, GLuint index, const GLfloat *v)
{
    INSTRUMENT();
    caching_client_glVertexAttrib3f(client, index, v[0], v[1], v[2]);
}

static void
caching_client_glVertexAttrib4fv (void* client, GLuint index, const GLfloat *v)
{
    INSTRUMENT();
    caching_client_glVertexAttrib4f(client, index, v[0], v[1], v[2], v[3]);
}

static void
caching_client_glVertexAttribPointer (void* client, GLuint index, GLint size,
                                      GLenum type, GLboolean normalized,
                                      GLsizei stride, const GLvoid *pointer)
{
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int i, found_index = -1;
    int count;
    GLint bound_buffer = 0;

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    attrib_list = &state->vertex_attribs;
    attribs = attrib_list->attribs;
    count = attrib_list->count;

    if (! (type == GL_BYTE                 ||
           type == GL_UNSIGNED_BYTE        ||
           type == GL_SHORT                ||
           type == GL_UNSIGNED_SHORT       ||
           type == GL_FLOAT                ||
           type == GL_FIXED)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }
    else if (size > 4 || size < 1 || stride < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    /* check max_vertex_attribs */
    if (caching_client_does_index_overflow (client, index))
        return;

    if (state->vertex_array_binding) {
        CACHING_CLIENT(client)->super_dispatch.glVertexAttribPointer (client, index, size,
                                                                      type, normalized, stride, pointer);
        /* FIXME: Do we need set this flag? */
        //state->need_get_error = true;
        return;
    }
    bound_buffer = state->array_buffer_binding;
    /* check existing client state */
    for (i = 0; i < count; i++) {
        if (attribs[i].index == index) {
            if (attribs[i].size == size &&
                attribs[i].type == type &&
                attribs[i].stride == stride &&
                attribs[i].array_buffer_binding == bound_buffer &&
                attribs[i].array_normalized == normalized &&
                attribs[i].pointer == pointer) {
                if (! bound_buffer && attribs[i].array_enabled) {
                    _set_vertex_pointers (attrib_list, &attribs[i]);
                }
                return;
            }
            else {
                found_index = i;
                break;
            }
        }
    }

    /* use array_buffer? */
    if (bound_buffer) {
        CACHING_CLIENT(client)->super_dispatch.glVertexAttribPointer (client, index, size,
                                                                      type, normalized, stride, pointer);
    }

    if (found_index != -1) {
        attribs[found_index].size = size;
        attribs[found_index].stride = stride;
        attribs[found_index].type = type;
        attribs[found_index].array_normalized = normalized;
        attribs[found_index].array_buffer_binding = bound_buffer;

        if (attrib_list->last_index_pointer == &attribs[found_index])
            attrib_list->last_index_pointer = 0;
        if (attrib_list->first_index_pointer == &attribs[found_index])
            attrib_list->first_index_pointer = 0;

        attribs[found_index].pointer = (GLvoid *)pointer;
        attrib_list->enabled_count = 0;
        for (i = 0; i < attrib_list->count; i++) {
            if (attribs[i].array_enabled && !attribs[i].array_buffer_binding) {
                attrib_list->enabled_count++;
                _set_vertex_pointers (attrib_list, &attribs[i]);
            }
        }
        return;
    }

    /* we have not found index */
    if (i < NUM_EMBEDDED) {
        attribs[i].index = index;
        attribs[i].size = size;
        attribs[i].stride = stride;
        attribs[i].type = type;
        attribs[i].array_normalized = normalized;
        attribs[i].pointer = (GLvoid *)pointer;
        attribs[i].data = NULL;
        attribs[i].array_enabled = GL_FALSE;
        attribs[i].array_buffer_binding = bound_buffer;
        attrib_list->count ++;
    }
    else {
        vertex_attrib_t *new_attribs = (vertex_attrib_t *)malloc (sizeof (vertex_attrib_t) * (count + 1));

        memcpy (new_attribs, attribs, (count+1) * sizeof (vertex_attrib_t));
        if (attribs != attrib_list->embedded_attribs)
            free (attribs);

        new_attribs[count].index = index;
        new_attribs[count].size = size;
        new_attribs[count].stride = stride;
        new_attribs[count].type = type;
        new_attribs[count].array_normalized = normalized;
        new_attribs[count].pointer = (GLvoid *)pointer;
        new_attribs[count].data = NULL;
        new_attribs[count].array_enabled = GL_FALSE;
        new_attribs[count].array_buffer_binding = bound_buffer;

        attrib_list->attribs = new_attribs;
        attrib_list->count ++;
    }
}

static void
caching_client_glRenderbufferStorage (void *client, GLenum target,
                                      GLenum internalformat,
                                      GLsizei width, GLsizei height)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (target != GL_RENDERBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    if (state->framebuffer_binding) {
        framebuffer_t *fb = egl_state_lookup_cached_framebuffer (state, 
                                              state->framebuffer_binding);
        if (fb)
            fb->complete = FRAMEBUFFER_COMPLETE_UNKNOWN;
    }
    
    CACHING_CLIENT(client)->super_dispatch.glRenderbufferStorage (client,
                                                                  target,
                                                                  internalformat,
                                                                  width,
                                                                  height);
    caching_client_set_needs_get_error (CLIENT (client));
}

static void
caching_client_glRenderbufferStorageMultisampleANGLE (void *client, 
                                      GLenum target,
                                      GLsizei samples,
                                      GLenum internalformat,
                                      GLsizei width, GLsizei height)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (target != GL_RENDERBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    if (state->framebuffer_binding) {
        framebuffer_t *fb = egl_state_lookup_cached_framebuffer (state, 
                                              state->framebuffer_binding);
        if (fb)
            fb->complete = FRAMEBUFFER_COMPLETE_UNKNOWN;
    }
    
    CACHING_CLIENT(client)->super_dispatch.glRenderbufferStorageMultisampleANGLE (client,
                                                                  target,
                                                                  samples,
                                                                  internalformat,
                                                                  width,
                                                                  height);
    caching_client_set_needs_get_error (CLIENT (client));
}

static void
caching_client_glRenderbufferStorageMultisampleAPPLE (void *client, 
                                      GLenum target,
                                      GLsizei samples, 
                                      GLenum internalformat,
                                      GLsizei width, GLsizei height)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (target != GL_RENDERBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    if (state->framebuffer_binding) {
        framebuffer_t *fb = egl_state_lookup_cached_framebuffer (state, 
                                              state->framebuffer_binding);
        if (fb)
            fb->complete = FRAMEBUFFER_COMPLETE_UNKNOWN;
    }
    
    CACHING_CLIENT(client)->super_dispatch.glRenderbufferStorageMultisampleAPPLE (client,
                                                                  target,
                                                                  samples,
                                                                  internalformat,
                                                                  width,
                                                                  height);
    caching_client_set_needs_get_error (CLIENT (client));
}

static void
caching_client_glRenderbufferStorageMultisampleEXT (void *client, 
                                      GLenum target,
                                      GLsizei samples,
                                      GLenum internalformat,
                                      GLsizei width, GLsizei height)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (target != GL_RENDERBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    if (state->framebuffer_binding) {
        framebuffer_t *fb = egl_state_lookup_cached_framebuffer (state, 
                                              state->framebuffer_binding);
        if (fb)
            fb->complete = FRAMEBUFFER_COMPLETE_UNKNOWN;
    }
    
    CACHING_CLIENT(client)->super_dispatch.glRenderbufferStorageMultisampleEXT (client,
                                                                  target,
                                                                  samples, 
                                                                  internalformat,
                                                                  width,
                                                                  height);
    caching_client_set_needs_get_error (CLIENT (client));
}

static void
caching_client_glRenderbufferStorageMultisampleIMG (void *client, 
                                      GLenum target,
                                      GLsizei samples,
                                      GLenum internalformat,
                                      GLsizei width, GLsizei height)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (target != GL_RENDERBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    if (state->framebuffer_binding) {
        framebuffer_t *fb = egl_state_lookup_cached_framebuffer (state, 
                                              state->framebuffer_binding);
        if (fb)
            fb->complete = FRAMEBUFFER_COMPLETE_UNKNOWN;
    }
    
    CACHING_CLIENT(client)->super_dispatch.glRenderbufferStorageMultisampleIMG (client,
                                                                  target,
                                                                  samples,
                                                                  internalformat,
                                                                  width,
                                                                  height);
    caching_client_set_needs_get_error (CLIENT (client));
}

static void
caching_client_glViewport (void* client, GLint x, GLint y, GLsizei width, GLsizei height)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    if (state->viewport[0] == x     &&
        state->viewport[1] == y     &&
        state->viewport[2] == width &&
        state->viewport[3] == height)
        return;

    if (width < 0 || height < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    state->viewport[0] = x;
    state->viewport[1] = y;
    state->viewport[2] = width;
    state->viewport[3] = height;

    CACHING_CLIENT(client)->super_dispatch.glViewport (client, x, y, width, height);
}

static void
caching_client_glEGLImageTargetTexture2DOES (void* client, GLenum target, GLeglImageOES image)
{
    INSTRUMENT();
    if (target != GL_TEXTURE_2D) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glEGLImageTargetTexture2DOES (client, target, image);
}

static void*
caching_client_glMapBufferOES (void* client, GLenum target, GLenum access)
{
    void *result = NULL;

    INSTRUMENT();
    if (access != GL_WRITE_ONLY_OES) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return result;
    }

    if (! is_valid_BufferTarget (target)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return result;
    }

    result = CACHING_CLIENT(client)->super_dispatch.glMapBufferOES (client, target, access);

    if (result == NULL)
        caching_client_set_needs_get_error (CLIENT (client));
    return result;
}

static GLboolean
caching_client_glUnmapBufferOES (void* client, GLenum target)
{
    GLboolean result = GL_FALSE;

    INSTRUMENT();

    if (! is_valid_BufferTarget (target)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return result;
    }

    result = CACHING_CLIENT(client)->super_dispatch.glUnmapBufferOES (client, target);

    if (result != GL_TRUE)
        caching_client_set_needs_get_error (CLIENT (client));
    return result;
}

static void
caching_client_glGetBufferPointervOES (void* client, GLenum target, GLenum pname, GLvoid **params)
{
    INSTRUMENT();

    if (! is_valid_BufferTarget (target)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glGetBufferPointervOES (client, target, pname, params);
}

static void
caching_client_glFramebufferTexture3DOES (void* client,
                                          GLenum target,
                                          GLenum attachment,
                                          GLenum textarget,
                                          GLuint texture,
                                          GLint level,
                                          GLint zoffset)
{
    texture_t *tex = NULL;
    framebuffer_t *framebuffer = NULL;
    GLuint framebuffer_id;

    INSTRUMENT();

    if (target != GL_FRAMEBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }
    
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    CACHING_CLIENT(client)->super_dispatch.glFramebufferTexture3DOES (client, target, attachment,
                                                                      textarget, texture, level, zoffset);
    
    /* get cached texture */
    tex = egl_state_lookup_cached_texture (state, texture);
    if (tex) {
        tex->framebuffer_id = state->framebuffer_binding;
        framebuffer_id = tex->framebuffer_id;
        /* update framebuffer in cache */
        if (framebuffer_id) {
            framebuffer = egl_state_lookup_cached_framebuffer (state, framebuffer_id);
            if (framebuffer) {
                framebuffer->complete = FRAMEBUFFER_COMPLETE_UNKNOWN;
                framebuffer->attached_image = texture;
            }
        }
    }
}

/* spec: http://www.hhronos.org/registry/gles/extensions/OES/OES_vertex_array_object.txt
 * spec says it generates GL_INVALID_OPERATION if
 * (1) array is not generated by glGenVertexArrayOES()
 * (2) the array object has been deleted by glDeleteVertexArrayOES()
 *
 * SPECIAL ATTENTION:
 * this call also affect glVertexAttribPointer() and glDrawXXXX(),
 * if there is a vertex_array_binding, instead of using client state,
 * we pass them to client.
 */
static void
caching_client_glBindVertexArrayOES (void* client, GLuint array)
{
    INSTRUMENT();

    /* FIXME: should we save this ? */
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;
    if (state->vertex_array_binding == array)
        return;
    state->vertex_array_binding = array;

    CACHING_CLIENT(client)->super_dispatch.glBindVertexArrayOES (client, array);
}

static void
caching_client_glDeleteVertexArraysOES (void* client, GLsizei n, const GLuint *arrays)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (n <= 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glDeleteVertexArraysOES (client, n, arrays);

    /* matching vertex_array_binding ? */
    int i;
    for (i = 0; i < n; i++) {
        if (arrays[i] == state->vertex_array_binding) {
            state->vertex_array_binding = 0;
            break;
        }
    }
}

static void
caching_client_glGenVertexArraysOES (void* client, GLsizei n, GLuint *arrays)
{
    INSTRUMENT();

    if (n <= 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glGenVertexArraysOES (client, n, arrays);
}

static GLboolean
caching_client_glIsVertexArrayOES (void* client, GLuint array)
{
    GLboolean result = GL_FALSE;

    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return GL_FALSE;

    result = CACHING_CLIENT(client)->super_dispatch.glIsVertexArrayOES (client, array);

    if (result == GL_FALSE &&
        state->vertex_array_binding == array)
        state->vertex_array_binding = 0;
    return result;
}

static void
caching_client_glDiscardFramebufferEXT (void* client,
                             GLenum target, GLsizei numAttachments,
                             const GLenum *attachments)
{
    int i;

    INSTRUMENT();

    if (target != GL_FRAMEBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    for (i = 0; i < numAttachments; i++) {
        if (! is_valid_Attachment (attachments[i])) {
            caching_client_glSetError (client, GL_INVALID_ENUM);
            return;
        }
    }

    CACHING_CLIENT(client)->super_dispatch.glDiscardFramebufferEXT (client, target, numAttachments, attachments);
}

static void
caching_client_glMultiDrawArraysEXT (void* client,
                                     GLenum mode,
                                     GLint *first,
                                     GLsizei *count,
                                     GLsizei primcount)
{
    /* FIXME: things a little complicated here.  We cannot simply
     * turn this into a sync call, because, we have not called
     * glVertexAttribPointer() yet
     */
    caching_client_glSetError (client, GL_INVALID_OPERATION);
}

void
caching_client_glMultiDrawElementsEXT (void* client, GLenum mode, const GLsizei *count, GLenum type,
                             const GLvoid **indices, GLsizei primcount)
{
    /* FIXME: things a little complicated here.  We cannot simply
     * turn this into a sync call, because, we have not called
     * glVertexAttribPointer() yet
     */
    caching_client_glSetError (client, GL_INVALID_OPERATION);
}

static void
caching_client_glFramebufferTexture2DMultisampleEXT (void* client, GLenum target,
                                            GLenum attachment,
                                            GLenum textarget,
                                            GLuint texture,
                                            GLint level, GLsizei samples)
{
    texture_t *tex = NULL;
    framebuffer_t *framebuffer = NULL;
    GLuint framebuffer_id;

    INSTRUMENT();
    
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;

    if (target != GL_FRAMEBUFFER) {
       caching_client_glSetError (client, GL_INVALID_ENUM);
       return;
    }

    CACHING_CLIENT(client)->super_dispatch.glFramebufferTexture2DMultisampleEXT (
        client, target, attachment, textarget, texture, level, samples);
    
    /* get cached texture */
    tex = egl_state_lookup_cached_texture (state, texture);
    if (tex) {
        tex->framebuffer_id = state->framebuffer_binding;
        framebuffer_id = tex->framebuffer_id;
        /* update framebuffer in cache */
        if (framebuffer_id) {
            framebuffer = egl_state_lookup_cached_framebuffer (state, framebuffer_id);
            if (framebuffer) {
                framebuffer->complete = FRAMEBUFFER_COMPLETE_UNKNOWN;
                framebuffer->attached_image = texture;
            }
        }
    }
}

static void
caching_client_glFramebufferTexture2DMultisampleIMG (void* client, GLenum target,
                                                     GLenum attachment,
                                                     GLenum textarget,
                                                     GLuint texture,
                                                     GLint level, GLsizei samples)
{
    texture_t *tex = NULL;
    framebuffer_t *framebuffer = NULL;
    GLuint framebuffer_id;
    
    INSTRUMENT();
    
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state)
        return;


    if (target != GL_FRAMEBUFFER) {
       caching_client_glSetError (client, GL_INVALID_ENUM);
       return;
    }

    CACHING_CLIENT(client)->super_dispatch.glFramebufferTexture2DMultisampleIMG (
        client, target, attachment, textarget, texture, level, samples);
    
    /* get cached texture */
    tex = egl_state_lookup_cached_texture (state, texture);
    if (tex) {
        tex->framebuffer_id = state->framebuffer_binding;
        framebuffer_id = tex->framebuffer_id;
        /* update framebuffer in cache */
        if (framebuffer_id) {
            framebuffer = egl_state_lookup_cached_framebuffer (state, framebuffer_id);
            if (framebuffer) {
                framebuffer->complete = FRAMEBUFFER_COMPLETE_UNKNOWN;
                framebuffer->attached_image = texture;
            }
        }
    }
}

static void
caching_client_glDeleteFencesNV (void* client, GLsizei n, const GLuint *fences)
{
    INSTRUMENT();

    if (n <= 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glDeleteFencesNV (client, n, fences);
}

static void
caching_client_glGenFencesNV (void* client, GLsizei n, GLuint *fences)
{
    INSTRUMENT();

    if (n <= 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glGenFencesNV (client, n, fences);
}

static GLboolean
caching_client_glTestFenceNV (void* client, GLuint fence)
{
    INSTRUMENT();

    GLboolean result = CACHING_CLIENT(client)->super_dispatch.glTestFenceNV (client, fence);

    if (result == GL_FALSE)
        caching_client_set_needs_get_error (CLIENT (client));
    return result;
}

static void
caching_client_glGetFenceivNV (void* client, GLuint fence, GLenum pname, int *params)
{
    int original_params = *params;

    INSTRUMENT();

    CACHING_CLIENT(client)->super_dispatch.glGetFenceivNV (client, fence, pname, params);

    if (original_params == *params)
        caching_client_set_needs_get_error (CLIENT (client));
}

static void
caching_client_glCoverageOperationNV (void* client, GLenum operation)
{
    INSTRUMENT();

    if (! (operation == GL_COVERAGE_ALL_FRAGMENTS_NV  ||
           operation == GL_COVERAGE_EDGE_FRAGMENTS_NV ||
           operation == GL_COVERAGE_AUTOMATIC_NV)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }
    CACHING_CLIENT(client)->super_dispatch.glCoverageOperationNV (client, operation);
}

static EGLDisplay
caching_client_eglGetDisplay (void *client,
                              NativeDisplayType native_display)
{
    EGLDisplay result = CACHING_CLIENT(client)->super_dispatch.eglGetDisplay (client, native_display);

    if (result != EGL_NO_DISPLAY && cached_gl_display_find (result) == NULL) {
        mutex_lock (cached_gl_display_list_mutex);
        display_ctxs_surfaces_t *dpy = cached_gl_display_new (native_display, result);
        link_list_t **dpys = cached_gl_displays ();
        link_list_append (dpys, (void *)dpy, destroy_dpy);
        mutex_unlock (cached_gl_display_list_mutex);
    }
    return result;
}

static char const *
caching_client_eglQueryString (void *client, EGLDisplay display,  EGLint name)
{
    const char *result = CACHING_CLIENT(client)->super_dispatch.eglQueryString (client, display, name);

    if (name == EGL_EXTENSIONS) {
        if (strstr (result, "EGL_KHR_surfaceless_context") ||
            strstr (result, "EGL_KHR_surfaceless_opengl")) {
            mutex_lock (cached_gl_display_list_mutex);
            link_list_t **dpys = cached_gl_displays ();
            link_list_t *dpy = *dpys;
            while (dpy) {
                display_ctxs_surfaces_t *d = (display_ctxs_surfaces_t *)dpy->data;
                if (d->display == display) {
                    d->support_surfaceless = true;
                    break;
                }
                dpy = dpy->next;
            }
            mutex_unlock (cached_gl_display_list_mutex);
        }
    }
    return result;
}

static EGLBoolean
caching_client_eglTerminate (void* client,
                             EGLDisplay display)
{
    INSTRUMENT();

    if (CACHING_CLIENT(client)->super_dispatch.eglTerminate (client, display) == EGL_FALSE)
        return EGL_FALSE;

    mutex_lock (cached_gl_states_mutex);

    link_list_t **states = cached_gl_states ();
    if (! *states) {
        mutex_unlock (cached_gl_states_mutex);
        return EGL_TRUE;
    }

    link_list_t *list = *states;
    while (list != NULL) {

        /* We might delete current in the following code. */
        link_list_t *current = list;
        list = current->next;

        egl_state_t *egl_state = (egl_state_t *) current->data;
        if (egl_state->display == display && ! egl_state->active)
            _caching_client_destroy_state (client, egl_state);
    }

    egl_state_t *egl_state = client_get_current_state (CLIENT (client));
    if (egl_state && egl_state->display == display)
        egl_state->destroy_dpy = true; /* Queue destroy later. */
    else {
        mutex_lock (cached_gl_display_list_mutex);
        cached_gl_display_destroy (display);
        mutex_unlock (cached_gl_display_list_mutex);
    }

    mutex_unlock (cached_gl_states_mutex);
    return EGL_TRUE;
}

static EGLBoolean
caching_client_eglDestroySurface (void* client,
                                  EGLDisplay display,
                                  EGLSurface surface)
{
    INSTRUMENT();

    if (!CLIENT(client)->active_state)
        return EGL_FALSE;

    EGLBoolean result = CACHING_CLIENT(client)->super_dispatch.eglDestroySurface (client, display, surface);
    if (result == EGL_TRUE) {
        /* update gl states */
        _caching_client_destroy_surface (client, display, surface);
    }

    return result;
}

static EGLBoolean
caching_client_eglReleaseThread (void* client)
{
    INSTRUMENT();

    if (CACHING_CLIENT(client)->super_dispatch.eglReleaseThread (client) == EGL_FALSE)
        return EGL_FALSE;

    _caching_client_make_current (client,
                                  EGL_NO_DISPLAY,
                                  EGL_NO_SURFACE,
                                  EGL_NO_SURFACE,
                                  EGL_NO_CONTEXT);
    return EGL_TRUE;
}

static EGLBoolean
caching_client_eglDestroyContext (void* client,
                                  EGLDisplay dpy,
                                  EGLContext ctx)
{
    INSTRUMENT();

    if (CACHING_CLIENT(client)->super_dispatch.eglDestroyContext (client, dpy, ctx) == EGL_FALSE)
        return EGL_FALSE; /* Failure, so do nothing locally. */

    /* Destroy our cached version of this context. */
    mutex_lock (cached_gl_states_mutex);

    egl_state_t *state = find_state_with_display_and_context (dpy, ctx);
    if (!state) {
        mutex_unlock (cached_gl_states_mutex);
        return EGL_TRUE;
    }

    if (! state->active) {
        _caching_client_destroy_state (client, state);
        mutex_lock (cached_gl_display_list_mutex);
        cached_gl_context_destroy (dpy, ctx);
        mutex_unlock (cached_gl_display_list_mutex);
    }
    else
        state->destroy_ctx = true;

    mutex_unlock (cached_gl_states_mutex);
    return EGL_TRUE;
}

static EGLContext
caching_client_eglGetCurrentContext (void* client)
{
    INSTRUMENT();

    if (!CLIENT(client)->active_state)
        return EGL_NO_CONTEXT;

    egl_state_t *state = client_get_current_state (CLIENT(client));
    return state->context;
}

static EGLDisplay
caching_client_eglGetCurrentDisplay (void* client)
{
    INSTRUMENT();

    if (!CLIENT(client)->active_state)
        return EGL_NO_DISPLAY;

    egl_state_t *state = client_get_current_state (CLIENT(client));
    return state->display;
}

static EGLSurface
caching_client_eglGetCurrentSurface (void* client,
                          EGLint readdraw)
{
    EGLSurface surface = EGL_NO_SURFACE;

    INSTRUMENT();

    if (!CLIENT(client)->active_state)
        return EGL_NO_SURFACE;

    egl_state_t *state = client_get_current_state (CLIENT(client));
    if (state->display == EGL_NO_DISPLAY || state->context == EGL_NO_CONTEXT)
        goto FINISH;

    if (readdraw == EGL_DRAW)
        surface = state->drawable;
    else
        surface = state->readable;
 FINISH:
    return surface;
}

static EGLBoolean
caching_client_eglSwapBuffers (void* client,
                               EGLDisplay display,
                               EGLSurface surface)
{
    INSTRUMENT();

    if (!CLIENT(client)->active_state)
        return EGL_FALSE;

    egl_state_t *state = client_get_current_state (CLIENT(client));
    if (! (state->display == display &&
           state->drawable == surface))
        return EGL_FALSE;

    EGLBoolean result = CACHING_CLIENT(client)->super_dispatch.eglSwapBuffers (client, display, surface);
    return result;
}

static EGLSurface
caching_client_eglCreatePbufferSurface (void *client,
                                        EGLDisplay display,
                                        EGLConfig config,
                                        EGLint const *attrib_list)
{
    EGLSurface result =
        CACHING_CLIENT(client)->super_dispatch.eglCreatePbufferSurface (client,
                                                            display,
                                                            config,
                                                            attrib_list);
    if (result) {
        mutex_lock (cached_gl_display_list_mutex);
        cached_gl_surface_add (display, config, result);
        mutex_unlock (cached_gl_display_list_mutex);
    }

    return result;
}

static EGLSurface
caching_client_eglCreatePixmapSurface (void *client,
                                       EGLDisplay display,
                                       EGLConfig config,
                                       NativePixmapType native_pixmap,
                                       EGLint const *attrib_list)
{
    EGLSurface result =
        CACHING_CLIENT(client)->super_dispatch.eglCreatePixmapSurface (client,
                                                            display,
                                                            config,
                                                            native_pixmap,
                                                            attrib_list);
    if (result) {
        mutex_lock (cached_gl_display_list_mutex);
        cached_gl_surface_add (display, config, result);
        mutex_unlock (cached_gl_display_list_mutex);
    }

    return result;
}

static EGLSurface
caching_client_eglCreateWindowSurface (void *client,
                                       EGLDisplay display,
                                       EGLConfig config,
                                       NativeWindowType native_window,
                                       EGLint const *attrib_list)
{
    EGLSurface result =
        CACHING_CLIENT(client)->super_dispatch.eglCreateWindowSurface (client,
                                                            display,
                                                            config,
                                                            native_window,
                                                            attrib_list);
    if (result) {
        mutex_lock (cached_gl_display_list_mutex);
        cached_gl_surface_add (display, config, result);
        mutex_unlock (cached_gl_display_list_mutex);
    }

    return result;
}

static EGLContext
caching_client_eglCreateContext (void *client,
                                 EGLDisplay dpy,
                                 EGLConfig config,
                                 EGLContext share_context,
                                 const EGLint* attrib_list)
{
    EGLContext result =
        CACHING_CLIENT(client)->super_dispatch.eglCreateContext (client,
                                                                 dpy,
                                                                 config,
                                                                 share_context,
                                                                 attrib_list);

    if (result == EGL_NO_CONTEXT)
        return result;

    mutex_lock (cached_gl_display_list_mutex);
    cached_gl_context_add (dpy, config, result);
    mutex_unlock (cached_gl_display_list_mutex);

    if (share_context != EGL_NO_CONTEXT) {
	mutex_lock (cached_gl_states_mutex);
        egl_state_t *new_state = _caching_client_get_or_create_state (dpy, result);
        new_state->share_context = _caching_client_get_or_create_state (dpy, share_context);
        new_state->share_context->contexts_sharing++;
	mutex_unlock (cached_gl_states_mutex);
    }
    return result;
}

static EGLBoolean
caching_client_eglMakeCurrent (void* client,
                               EGLDisplay display,
                               EGLSurface draw,
                               EGLSurface read,
                               EGLContext ctx)
{
    INSTRUMENT();

    /* First detect situations where we are not changing the context. */
    egl_state_t *current_state = client_get_current_state (CLIENT(client));

    /* Switching from none to none, so this is a no-op. */
    bool switching_to_none = display == EGL_NO_DISPLAY || ctx == EGL_NO_CONTEXT;
    if (switching_to_none && ! current_state)
        return EGL_TRUE;

    /* Everything matches, so this is a no-op. */
    egl_state_t *matching_state = NULL;
    bool find_draw, find_read;

    bool display_and_context_match = current_state &&
                                     current_state->display == display &&
                                     current_state->context == ctx;
    if (display_and_context_match) {
        if (current_state->drawable == draw &&
            current_state->readable == read) {
            return EGL_TRUE;
        }

        mutex_lock (cached_gl_states_mutex);
 
        link_list_t **surfaces = cached_gl_surfaces (display);

        matching_state = current_state;
        find_draw = cached_gl_surface_match (surfaces, draw);
        if (! find_draw)
            matching_state = NULL;

        find_read = cached_gl_surface_match (surfaces, read);
        if (! find_read)
            matching_state = NULL;

        mutex_unlock (cached_gl_states_mutex);
    }

    /* We have found previously used draw and read surface.  Because
     * the display and context are matching, it means we are not
     * releasing the context in the thread, we can do async
     */
    if (display_and_context_match && matching_state) {
        command_t *command = client_get_space_for_command (COMMAND_EGLMAKECURRENT);
        command_eglmakecurrent_init (command, display, draw, read, ctx);
        client_run_command_async (command);
    } else {
        /* Otherwise we must do this synchronously. */
        if (CACHING_CLIENT(client)->super_dispatch.eglMakeCurrent (client, display,
                                                                   draw, read, ctx) == EGL_FALSE)
            return EGL_FALSE; /* Don't do anything else if we fail. */
    }

    _caching_client_make_current (client, display, draw, read, ctx);
    return EGL_TRUE;
}

#include "caching_client_glget.c"

static void
caching_client_init (caching_client_t *client)
{
    client_init (&client->super);
    client->super_dispatch = client->super.dispatch;

    /* Initialize the cached GL states. */
    mutex_lock (cached_gl_states_mutex);
    cached_gl_states ();
    mutex_unlock (cached_gl_states_mutex);

    #include "caching_client_dispatch_autogen.c"
}

caching_client_t *
caching_client_new ()
{
    caching_client_t *client = (caching_client_t *)malloc (sizeof (caching_client_t));
    caching_client_init (client);
    return client;
}

void
caching_client_destroy (caching_client_t *client)
{
    client_destroy ((client_t *)client);
}
