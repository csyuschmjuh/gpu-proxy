#include "config.h"

#include "caching_client.h"
#include "caching_client_private.h"
#include "client.h"
#include "command.h"
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

static bool
caching_client_does_index_overflow (void* client,
                                    GLuint index)
{
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (index <= state->max_vertex_attribs)
        return false;

    if (! state->max_vertex_attribs_queried) {
        CACHING_CLIENT(client)->super_dispatch.glGetIntegerv (client, GL_MAX_VERTEX_ATTRIBS,
                                                              &state->max_vertex_attribs);
        state->max_vertex_attribs_queried = true;
    }

    if (index <= state->max_vertex_attribs)
        return false;
    if (state->error == GL_NO_ERROR)
        state->error = GL_INVALID_VALUE;
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

void
delete_texture_from_name_handler (GLuint key,
                                  void *data,
                                  void *userData)
{
    name_handler_delete_names (1, data);
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

    /* We don't use egl_state_get_texture_cache here because we don't want to
     * delete a sharing context's texture when we are cleaning up a client context. */
    HashWalk (egl_state->texture_cache,
              delete_texture_from_name_handler,
              NULL);
    egl_state_destroy (egl_state);
    link_list_delete_first_entry_matching_data (cached_gl_states (), egl_state);
}

static egl_state_t *
find_state_with_display_and_context (EGLDisplay display,
                                     EGLContext context,
                                     bool hold_mutex)
{
    if (hold_mutex)
        mutex_lock (cached_gl_states_mutex);

    link_list_t **states = cached_gl_states ();

    link_list_t *current = *states;
    while (current) {
        egl_state_t *state = (egl_state_t *)current->data;
        if (state->context == context && state->display == display) {
            if (hold_mutex)
                mutex_unlock (cached_gl_states_mutex);
            return current->data;
        }
        current = current->next;
    }

    if (hold_mutex)
        mutex_unlock (cached_gl_states_mutex);
    return NULL;
}

static egl_state_t *
_caching_client_get_or_create_state (EGLDisplay dpy,
                                     EGLContext ctx)
{
    /* We should already be holding the cached states mutex. */
    egl_state_t *state = find_state_with_display_and_context(dpy, ctx, false);
    if (state)
        return state;

    state = egl_state_new ();
    state->context = ctx;
    state->display = dpy;
    link_list_append (cached_gl_states (), state);
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
        current_state->active = false;

        if (current_state->destroy_read)
            current_state->readable = EGL_NO_SURFACE;
        if (current_state->destroy_draw)
            current_state->drawable = EGL_NO_SURFACE;

        if (current_state->destroy_dpy || current_state->destroy_ctx)
            _caching_client_destroy_state (client, current_state);
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
            }
        }
    }

    mutex_unlock (cached_gl_states_mutex);
}

static void
_set_vertex_pointers (vertex_attrib_list_t *list,
                      vertex_attrib_t *new_attrib)
{
    void *pointer = new_attrib->pointer;
    if (! list->first_index_pointer || pointer < list->first_index_pointer->pointer)
        list->first_index_pointer = new_attrib;
    if ( ! list->last_index_pointer || pointer > list->last_index_pointer->pointer)
        list->last_index_pointer = new_attrib;
}

static void
caching_client_glSetError (void* client, GLenum error)
{
    egl_state_t *egl_state = client_get_current_state (CLIENT(client));
    if (egl_state->active && egl_state->error == GL_NO_ERROR)
        egl_state->error = error;
}

/* GLES2 core profile API */
static void
caching_client_glActiveTexture (void* client,
                                GLenum texture)
{
    int texture_offset = texture - GL_TEXTURE0;

    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (state->active_texture == texture)
        return;
    else if (texture > GL_TEXTURE31 || texture < GL_TEXTURE0) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;

    } else {
        CACHING_CLIENT(client)->super_dispatch.glActiveTexture (client, texture);
        /* FIXME: this maybe not right because this texture may be 
         * invalid object, we save here to save time in glGetError() 
         */
        state->active_texture = texture;
        state->texture_binding[0] = -1;
        state->texture_binding[1] = -1;
        state->texture_binding_3d = -1;

        state->texture_mag_filter[texture_offset][0] = -1;
        state->texture_mag_filter[texture_offset][1] = -1;
        state->texture_mag_filter[texture_offset][2] = -1;
        state->texture_min_filter[texture_offset][0] = -1;
        state->texture_min_filter[texture_offset][1] = -1;
        state->texture_min_filter[texture_offset][2] = -1;
        state->texture_wrap_s[texture_offset][0] = -1;
        state->texture_wrap_s[texture_offset][1] = -1;
        state->texture_wrap_s[texture_offset][2] = -1;
        state->texture_wrap_t[texture_offset][0] = -1;
        state->texture_wrap_t[texture_offset][1] = -1;
        state->texture_wrap_t[texture_offset][2] = -1;
        state->texture_3d_wrap_r[texture_offset] = -1;
    }
}

static void
caching_client_glBindBuffer (void* client, GLenum target, GLuint buffer)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (target == GL_ARRAY_BUFFER) {
        if (state->array_buffer_binding == buffer)
            return;
        else {
            CACHING_CLIENT(client)->super_dispatch.glBindBuffer (client, target, buffer);
            caching_client_set_needs_get_error (CLIENT (client));

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

    if (target != GL_RENDERBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
    }

    CACHING_CLIENT(client)->super_dispatch.glBindRenderbuffer (client, target, renderbuffer);
    /* FIXME: should we save it, it will be invalid if the
     * renderbuffer is invalid 
     */
    egl_state_t *state = client_get_current_state (CLIENT (client));
    state->renderbuffer_binding = renderbuffer;
}

static void
caching_client_glBindTexture (void* client, GLenum target, GLuint texture)
{
    texture_t *tex;

    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
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
    if (texture != 0) {
        tex = (texture_t *) HashLookup (egl_state_get_texture_cache (state), texture);
        if (! tex) {
            caching_client_glSetError (client, GL_INVALID_OPERATION);
            return;
        }
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
    if (state->blend_equation[0] == mode &&
        state->blend_equation[1] == mode)
        return;

    if (! (mode == GL_FUNC_ADD ||
           mode == GL_FUNC_SUBTRACT ||
           mode == GL_FUNC_REVERSE_SUBTRACT)) {
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
    if (state->blend_equation[0] == modeRGB &&
        state->blend_equation[1] == modeAlpha)
        return;

    if (! (modeRGB == GL_FUNC_ADD ||
           modeRGB == GL_FUNC_SUBTRACT ||
           modeRGB == GL_FUNC_REVERSE_SUBTRACT) || 
        ! (modeAlpha == GL_FUNC_ADD ||
           modeAlpha == GL_FUNC_SUBTRACT ||
           modeAlpha == GL_FUNC_REVERSE_SUBTRACT)) {
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
    if (state->blend_src[0] == sfactor &&
        state->blend_src[1] == sfactor &&
        state->blend_dst[0] == dfactor &&
        state->blend_dst[1] == dfactor)
        return;

    if (! (sfactor == GL_ZERO ||
           sfactor == GL_ONE ||
           sfactor == GL_SRC_COLOR ||
           sfactor == GL_ONE_MINUS_SRC_COLOR ||
           sfactor == GL_DST_COLOR ||
           sfactor == GL_ONE_MINUS_DST_COLOR ||
           sfactor == GL_SRC_ALPHA ||
           sfactor == GL_ONE_MINUS_SRC_ALPHA ||
           sfactor == GL_DST_ALPHA ||
           sfactor == GL_ONE_MINUS_DST_ALPHA ||
           sfactor == GL_CONSTANT_COLOR ||
           sfactor == GL_ONE_MINUS_CONSTANT_COLOR ||
           sfactor == GL_CONSTANT_ALPHA ||
           sfactor == GL_ONE_MINUS_CONSTANT_ALPHA ||
           sfactor == GL_SRC_ALPHA_SATURATE) ||
        ! (dfactor == GL_ZERO ||
           dfactor == GL_ONE ||
           dfactor == GL_SRC_COLOR ||
           dfactor == GL_ONE_MINUS_SRC_COLOR ||
           dfactor == GL_DST_COLOR ||
           dfactor == GL_ONE_MINUS_DST_COLOR ||
           dfactor == GL_SRC_ALPHA ||
           dfactor == GL_ONE_MINUS_SRC_ALPHA ||
           dfactor == GL_DST_ALPHA ||
           dfactor == GL_ONE_MINUS_DST_ALPHA ||
           dfactor == GL_CONSTANT_COLOR ||
           dfactor == GL_ONE_MINUS_CONSTANT_COLOR ||
           dfactor == GL_CONSTANT_ALPHA ||
           dfactor == GL_ONE_MINUS_CONSTANT_ALPHA ||
           dfactor == GL_SRC_ALPHA_SATURATE)) {
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
    if (state->blend_src[0] == srcRGB &&
        state->blend_src[1] == srcAlpha &&
        state->blend_dst[0] == dstRGB &&
        state->blend_dst[1] == dstAlpha)
        return;

    if (! (srcRGB == GL_ZERO ||
           srcRGB == GL_ONE ||
           srcRGB == GL_SRC_COLOR ||
           srcRGB == GL_ONE_MINUS_SRC_COLOR ||
           srcRGB == GL_DST_COLOR ||
           srcRGB == GL_ONE_MINUS_DST_COLOR ||
           srcRGB == GL_SRC_ALPHA ||
           srcRGB == GL_ONE_MINUS_SRC_ALPHA ||
           srcRGB == GL_DST_ALPHA ||
           srcRGB == GL_ONE_MINUS_DST_ALPHA ||
           srcRGB == GL_CONSTANT_COLOR ||
           srcRGB == GL_ONE_MINUS_CONSTANT_COLOR ||
           srcRGB == GL_CONSTANT_ALPHA ||
           srcRGB == GL_ONE_MINUS_CONSTANT_ALPHA ||
           srcRGB == GL_SRC_ALPHA_SATURATE) ||
        ! (dstRGB == GL_ZERO ||
           dstRGB == GL_ONE ||
           dstRGB == GL_SRC_COLOR ||
           dstRGB == GL_ONE_MINUS_SRC_COLOR ||
           dstRGB == GL_DST_COLOR ||
           dstRGB == GL_ONE_MINUS_DST_COLOR ||
           dstRGB == GL_SRC_ALPHA ||
           dstRGB == GL_ONE_MINUS_SRC_ALPHA ||
           dstRGB == GL_DST_ALPHA ||
           dstRGB == GL_ONE_MINUS_DST_ALPHA ||
           dstRGB == GL_CONSTANT_COLOR ||
           dstRGB == GL_ONE_MINUS_CONSTANT_COLOR ||
           dstRGB == GL_CONSTANT_ALPHA ||
           dstRGB == GL_ONE_MINUS_CONSTANT_ALPHA ||
           dstRGB == GL_SRC_ALPHA_SATURATE) ||
        ! (srcAlpha == GL_ZERO ||
           srcAlpha == GL_ONE ||
           srcAlpha == GL_SRC_COLOR ||
           srcAlpha == GL_ONE_MINUS_SRC_COLOR ||
           srcAlpha == GL_DST_COLOR ||
           srcAlpha == GL_ONE_MINUS_DST_COLOR ||
           srcAlpha == GL_SRC_ALPHA ||
           srcAlpha == GL_ONE_MINUS_SRC_ALPHA ||
           srcAlpha == GL_DST_ALPHA ||
           srcAlpha == GL_ONE_MINUS_DST_ALPHA ||
           srcAlpha == GL_CONSTANT_COLOR ||
           srcAlpha == GL_ONE_MINUS_CONSTANT_COLOR ||
           srcAlpha == GL_CONSTANT_ALPHA ||
           srcAlpha == GL_ONE_MINUS_CONSTANT_ALPHA ||
           srcAlpha == GL_SRC_ALPHA_SATURATE) ||
        ! (dstAlpha == GL_ZERO ||
           dstAlpha == GL_ONE ||
           dstAlpha == GL_SRC_COLOR ||
           dstAlpha == GL_ONE_MINUS_SRC_COLOR ||
           dstAlpha == GL_DST_COLOR ||
           dstAlpha == GL_ONE_MINUS_DST_COLOR ||
           dstAlpha == GL_SRC_ALPHA ||
           dstAlpha == GL_ONE_MINUS_SRC_ALPHA ||
           dstAlpha == GL_DST_ALPHA ||
           dstAlpha == GL_ONE_MINUS_DST_ALPHA ||
           dstAlpha == GL_CONSTANT_COLOR ||
           dstAlpha == GL_ONE_MINUS_CONSTANT_COLOR ||
           dstAlpha == GL_CONSTANT_ALPHA ||
           dstAlpha == GL_ONE_MINUS_CONSTANT_ALPHA ||
           dstAlpha == GL_SRC_ALPHA_SATURATE)) {
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
    INSTRUMENT();

    if (target != GL_FRAMEBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return GL_INVALID_ENUM;
    }

    return CACHING_CLIENT(client)->super_dispatch.glCheckFramebufferStatus (client, target);
}

static void
caching_client_glClear (void* client, GLbitfield mask)
{
    INSTRUMENT();

    if (! (mask & GL_COLOR_BUFFER_BIT ||
           mask & GL_DEPTH_BUFFER_BIT ||
           mask & GL_STENCIL_BUFFER_BIT)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glClear (client, mask);
}

static void
caching_client_glClearColor (void* client, GLclampf red, GLclampf green,
                 GLclampf blue, GLclampf alpha)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (state->color_clear_value[0] == red &&
        state->color_clear_value[1] == green &&
        state->color_clear_value[2] == blue &&
        state->color_clear_value[3] == alpha)
        return;

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
    if (state->depth_clear_value == depth)
        return;

    state->depth_clear_value = depth;

    CACHING_CLIENT(client)->super_dispatch.glClearDepthf (client, depth);
}

static void
caching_client_glClearStencil (void* client, GLint s)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (state->stencil_clear_value == s)
        return;

    state->stencil_clear_value = s;
    CACHING_CLIENT(client)->super_dispatch.glClearStencil (client, s);
}

static void
caching_client_glColorMask (void* client, GLboolean red, GLboolean green,
                GLboolean blue, GLboolean alpha)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
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

static GLuint
caching_client_glCreateProgram (void* client)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));

    GLuint result = CACHING_CLIENT(client)->super_dispatch.glCreateProgram (client);
    if (result == 0)
        caching_client_set_needs_get_error (CLIENT (client));

    program_t *new_program = (program_t *)malloc (sizeof (program_t));
    new_program->id = result; 
    new_program->mark_for_deletion = false;
    new_program->uniform_location_cache = NewHashTable(free);
    new_program->attrib_location_cache = NewHashTable(free);
    link_list_append (&state->programs, new_program);

    return result;
}

static void
caching_client_glDeleteProgram (void *client,
                                GLuint program)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));

    link_list_t *cached_program_item = state->programs;
    program_t *cached_program = NULL;
    while (cached_program_item) {
        cached_program = (program_t *) cached_program_item->data;
        if (cached_program->id == program)
            break;
        cached_program_item = cached_program_item->next;
    }

    if (! cached_program_item) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glDeleteProgram (client, program);

    if (cached_program->id == state->current_program) {
        cached_program->mark_for_deletion = true;
        return;
    }

    DeleteHashTable (cached_program->attrib_location_cache);
    DeleteHashTable (cached_program->uniform_location_cache);

    link_list_delete_element (&state->programs, cached_program_item);
}

static GLuint
caching_client_glCreateShader (void* client, GLenum shaderType)
{
    INSTRUMENT();

    if (! (shaderType == GL_VERTEX_SHADER ||
           shaderType == GL_FRAGMENT_SHADER)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return 0;
    }

    GLuint result = CACHING_CLIENT(client)->super_dispatch.glCreateShader (client, shaderType);

    if (result == 0)
        caching_client_set_needs_get_error (CLIENT (client));
    return result;
}

static void
caching_client_glCullFace (void* client, GLenum mode)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (state->cull_face_mode == mode)
        return;

    if (! (mode == GL_FRONT ||
           mode == GL_BACK ||
           mode == GL_FRONT_AND_BACK)) {
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
    vertex_attrib_list_t *attrib_list = &state->vertex_attribs;
    vertex_attrib_t *attribs = attrib_list->attribs;
    count = attrib_list->count;

    if (n < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glDeleteBuffers (client, n, buffers);

    name_handler_delete_names (n, buffers);

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
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));

    if (n < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    name_handler_delete_names (n, framebuffers);

    CACHING_CLIENT(client)->super_dispatch.glDeleteFramebuffers (client, n, framebuffers);

    int i;
    for (i = 0; i < n; i++) {
        if (state->framebuffer_binding == framebuffers[i]) {
            state->framebuffer_binding = 0;
            break;
        }
    }
}

static void
caching_client_glDeleteRenderbuffers (void* client, GLsizei n, const GLuint *renderbuffers)
{
    INSTRUMENT();

    if (n < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    name_handler_delete_names (n, renderbuffers);

    CACHING_CLIENT(client)->super_dispatch.glDeleteRenderbuffers (client, n, renderbuffers);
}

static void
caching_client_glDeleteTextures (void* client, GLsizei n, const GLuint *textures)
{
    INSTRUMENT();

    if (n < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glDeleteTextures (client, n, textures);

    name_handler_delete_names (n, textures);
}

static void
caching_client_glDepthFunc (void* client, GLenum func)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (state->depth_func == func)
        return;

    if (! (func == GL_NEVER ||
           func == GL_LESS ||
           func == GL_EQUAL ||
           func == GL_LEQUAL ||
           func == GL_GREATER ||
           func == GL_NOTEQUAL ||
           func == GL_GEQUAL ||
           func == GL_ALWAYS)) {
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
        if (enable)
           CACHING_CLIENT(client)->super_dispatch.glEnableVertexAttribArray (client, index);
        else
           CACHING_CLIENT(client)->super_dispatch.glDisableVertexAttribArray (client, index);
        caching_client_set_needs_get_error (CLIENT (client));
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
    if (caching_client_does_index_overflow (client, index)) {
        return;
    }

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
                _set_vertex_pointers (attrib_list, &attribs[found_index]);
            }
            else {
                if (attribs[found_index].pointer == attrib_list->first_index_pointer)
                    attrib_list->first_index_pointer = 0;
                if (attribs[found_index].pointer == attrib_list->last_index_pointer)
                    attrib_list->last_index_pointer = 0;
            
                for (i = 0; i < attrib_list->count; i++) {
                    if (attribs[i].array_enabled && !attribs[i].array_buffer_binding)
                        _set_vertex_pointers (attrib_list, &attribs[i]);
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
        memset ((void *)attribs[i].current_attrib, 0, sizeof (GLfloat) * 4);
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
        memset (new_attribs[count].current_attrib, 0, sizeof (GLfloat) * 4);
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
    caching_client_glSetVertexAttribArray (client, index, state, GL_FALSE);
}

static void
caching_client_glEnableVertexAttribArray (void* client, GLuint index)
{
    INSTRUMENT();
    egl_state_t *state = client_get_current_state (CLIENT (client));
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
    vertex_attrib_list_t *attrib_list = &state->vertex_attribs;

    int i = -1;
    for (i = 0; i < attrib_list->count; i++)
        attrib_list->attribs[i].data = NULL;
}

static void
prepend_element_to_list (link_list_t **list,
                         void *element)
{
    link_list_t *new_list = (link_list_t *) malloc (sizeof (link_list_t));
    new_list->data = element;
    if (*list) {
        new_list->next = *list;
        new_list->prev = (*list)->prev;
        (*list)->prev = new_list;
    } else {
        new_list->next = new_list;
        new_list->prev = new_list;
    }
    *list = new_list;
}

static bool
client_has_vertex_attrib_array_set (client_t *client)
{
    egl_state_t *state = client_get_current_state (CLIENT (client));
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
    vertex_attrib_list_t *attrib_list = &state->vertex_attribs;

    vertex_attrib_t *last_pointer = attrib_list->last_index_pointer;
    if (! last_pointer)
        return false;
    if (! last_pointer->array_enabled || last_pointer->array_buffer_binding)
        return false;

    int data_size = _get_data_size (last_pointer->type);
    if (data_size == 0)
        return false;

    unsigned int stride = last_pointer->stride ? last_pointer->stride :
                                                 (last_pointer->size * data_size);
    return stride * count + (char *)last_pointer->pointer - (char*) attrib_list->first_index_pointer->pointer;
}

static void
caching_client_setup_vertex_attrib_pointer_if_necessary (client_t *client,
                                                         size_t count,
                                                         link_list_t **allocated_data_arrays)
{
    int i = 0;

    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    vertex_attrib_list_t *attrib_list = &state->vertex_attribs;
    vertex_attrib_t *attribs = attrib_list->attribs;

    size_t array_size = caching_client_vertex_attribs_array_size (client, count);
    if (!array_size)
        return;

    char *one_array_data = NULL;
    bool fits_in_one_array = array_size < ATTRIB_BUFFER_SIZE;
    if (fits_in_one_array) {
        if (array_size <= 1024 && client->last_1k_index < MEM_1K_SIZE) {
            one_array_data = client->pre_1k_mem[client->last_1k_index];
            client->last_1k_index++;
        }
        else if (array_size <= 2048 && client->last_2k_index < MEM_2K_SIZE) {
            one_array_data = client->pre_2k_mem[client->last_2k_index];
            client->last_2k_index++;
        }
        else if (array_size <= 4096 && client->last_4k_index < MEM_4K_SIZE) {
            one_array_data = client->pre_4k_mem[client->last_4k_index];
            client->last_4k_index++;
        }
        else if (array_size <= 8192 && client->last_8k_index < MEM_8K_SIZE) {
            one_array_data = client->pre_8k_mem[client->last_8k_index];
            client->last_8k_index++;
        }
        else if (array_size <= 16384 && client->last_16k_index < MEM_16K_SIZE) {
            one_array_data = client->pre_16k_mem[client->last_16k_index];
            client->last_16k_index++;
        }
        else if (array_size <= 32768 && client->last_32k_index < MEM_32K_SIZE) {
            one_array_data = client->pre_32k_mem[client->last_32k_index];
            client->last_32k_index++;
        }
        else {
            one_array_data = (char *)malloc (array_size);
            prepend_element_to_list (allocated_data_arrays, one_array_data);
        }
        memcpy (one_array_data,
                attrib_list->first_index_pointer->pointer,
                array_size);
    }

    for (i = 0; i < attrib_list->count; i++) {
        if (! attribs[i].array_enabled || attribs[i].array_buffer_binding)
            continue;

        /* We need to create a separate buffer for it. */
        if (fits_in_one_array) {
            attribs[i].data = (char *)attribs[i].pointer - (char*)attrib_list->first_index_pointer->pointer + one_array_data;
        } else {
            attribs[i].data = _create_data_array (&attribs[i], count);
            if (! attribs[i].data)
                continue;

            prepend_element_to_list (allocated_data_arrays, attribs[i].data);
        }
        command_t *command = client_get_space_for_command (COMMAND_GLVERTEXATTRIBPOINTER);
        if (fits_in_one_array)
            command_glvertexattribpointer_init (command, 
                                            attribs[i].index,
                                            attribs[i].size,
                                            attribs[i].type,
                                            attribs[i].array_normalized,
                                            attribs[i].stride,
                                            (const void *)attribs[i].data);
        else
            command_glvertexattribpointer_init (command, 
                                            attribs[i].index,
                                            attribs[i].size,
                                            attribs[i].type,
                                            attribs[i].array_normalized,
                                            0,
                                            (const void *)attribs[i].data);
        client_run_command_async (command);
    }
}

static void
caching_client_glDrawArrays (void* client,
                             GLenum mode,
                             GLint first,
                             GLsizei count)
{
    INSTRUMENT();

    if (! (mode == GL_POINTS         ||
           mode == GL_LINE_STRIP     ||
           mode == GL_LINE_LOOP      ||
           mode == GL_LINES          ||
           mode == GL_TRIANGLE_STRIP ||
           mode == GL_TRIANGLE_FAN   ||
           mode == GL_TRIANGLES)) {
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

    /* If vertex array binding is 0 and we have not bound a vertex attrib pointer, we
     * don't need to do anything. */
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state->vertex_array_binding && ! client_has_vertex_attrib_array_set (CLIENT (client))) {
        caching_client_clear_attribute_list_data (CLIENT(client));
        return;
    }

    link_list_t *arrays_to_free = NULL;
    if (! state->vertex_array_binding) {
        size_t true_count = first > 0 ? first + count : count;
        caching_client_setup_vertex_attrib_pointer_if_necessary (CLIENT(client),
                                                                 true_count,
                                                                 &arrays_to_free);
    }

    command_t *command = client_get_space_for_command (COMMAND_GLDRAWARRAYS);
    command_gldrawarrays_init (command, mode, first, count);
    ((command_gldrawarrays_t *) command)->arrays_to_free = arrays_to_free;
     client_run_command_async (command);

    caching_client_clear_attribute_list_data (CLIENT(client));
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
    if (! (mode == GL_POINTS         ||
           mode == GL_LINE_STRIP     ||
           mode == GL_LINE_LOOP      ||
           mode == GL_LINES          ||
           mode == GL_TRIANGLE_STRIP ||
           mode == GL_TRIANGLE_FAN   ||
           mode == GL_TRIANGLES)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        caching_client_clear_attribute_list_data (CLIENT(client));
        return;
    }
    if (! (type == GL_UNSIGNED_BYTE  || type == GL_UNSIGNED_SHORT)) {
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
    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (! state->vertex_array_binding && ! client_has_vertex_attrib_array_set (CLIENT (client))) {
        caching_client_clear_attribute_list_data (CLIENT(client));
        return;
    }

    /* If we aren't actually passing any indices then do not execute anything. */
    bool copy_indices = !state->vertex_array_binding && state->element_array_buffer_binding == 0;
    size_t index_array_size = calculate_index_array_size (type, count);
    if (! indices || (copy_indices && ! index_array_size)) {
        caching_client_clear_attribute_list_data (CLIENT(client));
        return;
    }

    size_t elements_count = _get_elements_count (type, indices, count);
    link_list_t *arrays_to_free = NULL;
         caching_client_setup_vertex_attrib_pointer_if_necessary (
            CLIENT (client), elements_count, &arrays_to_free);

    char* indices_to_pass = (char*) indices;
    command_gldrawelements_t *command = NULL;

    if (copy_indices) {
        command = (command_gldrawelements_t *)
            client_try_get_space_for_command_with_extra_space (COMMAND_GLDRAWELEMENTS,
                                                               index_array_size);
        if (command)
            indices_to_pass = ((char *) command) + command_get_size (COMMAND_GLDRAWELEMENTS);
        else {
            indices_to_pass = malloc (index_array_size);
            prepend_element_to_list (&arrays_to_free, indices_to_pass);
        }
        memcpy (indices_to_pass, indices, index_array_size);
    }

    if (! command)
        command = (command_gldrawelements_t *) client_get_space_for_command (COMMAND_GLDRAWELEMENTS);

    command_gldrawelements_init (&command->header, mode, count, type, indices_to_pass);
    ((command_gldrawelements_t *) command)->arrays_to_free = arrays_to_free;
    client_run_command_async (&command->header);

    caching_client_clear_attribute_list_data (CLIENT(client));
}

static void
caching_client_glFramebufferRenderbuffer (void* client, GLenum target, GLenum attachment,
                                          GLenum renderbuffertarget,
                                          GLenum renderbuffer)
{
    INSTRUMENT();

    if (target != GL_FRAMEBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    } else if (renderbuffertarget != GL_RENDERBUFFER && renderbuffer != 0) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glFramebufferRenderbuffer (client, target, attachment, renderbuffertarget, renderbuffer);
    caching_client_set_needs_get_error (CLIENT (client));
}

static void
caching_client_glFramebufferTexture2D (void* client,
                                       GLenum target,
                                       GLenum attachment,
                                       GLenum textarget,
                                       GLuint texture,
                                       GLint level)
{
    INSTRUMENT();

    if (target != GL_FRAMEBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glFramebufferTexture2D (client, target, attachment,
                                                                   textarget, texture, level);
    caching_client_set_needs_get_error (CLIENT (client));
}

static void
caching_client_glFrontFace (void* client, GLenum mode)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
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
    GLuint *server_buffers;
    int i;

    INSTRUMENT();

    if (n < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    name_handler_alloc_names (n, buffers);

    server_buffers = (GLuint *)malloc (n * sizeof (GLuint));
    for (i=0; i<n; i++)
        server_buffers[i] = buffers [i];

    CACHING_CLIENT(client)->super_dispatch.glGenBuffers (client, n, server_buffers);
}

static void
caching_client_glGenFramebuffers (void* client, GLsizei n, GLuint *framebuffers)
{
    GLuint *server_framebuffers;
    int i;

    INSTRUMENT();

    if (n < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    name_handler_alloc_names (n, framebuffers);

    server_framebuffers = (GLuint *)malloc (n * sizeof (GLuint));
    for (i=0; i<n; i++)
        server_framebuffers[i] = framebuffers [i];

    CACHING_CLIENT(client)->super_dispatch.glGenFramebuffers (client, n, server_framebuffers);
}

static void
caching_client_glGenRenderbuffers (void* client, GLsizei n, GLuint *renderbuffers)
{
    INSTRUMENT();

    if (n < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    name_handler_alloc_names (n, renderbuffers);

    GLuint *server_renderbuffers = (GLuint *)malloc (n * sizeof (GLuint));
    int i;
    for (i = 0; i < n; i++)
        server_renderbuffers[i] = renderbuffers [i];

    CACHING_CLIENT(client)->super_dispatch.glGenRenderbuffers (client, n, server_renderbuffers);
}

static texture_t *
_create_texture (GLuint id)
{
    texture_t *tex = (texture_t *) malloc (sizeof (texture_t));
    tex->id = id;
    tex->width = 0;
    tex->height = 0;
    tex->data_type = GL_UNSIGNED_BYTE;
    tex->internal_format = GL_RGBA;

    return tex;
}
    
static void
caching_client_glGenTextures (void* client, GLsizei n, GLuint *textures)
{
    GLuint *server_textures;
    int i;


    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));

    if (n < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    name_handler_alloc_names (n, textures);

    server_textures = (GLuint *)malloc (n * sizeof (GLuint));
    for (i = 0; i < n; i++)
        server_textures[i] = textures [i];

    /* FIXME:  we need to generate a list of client id for texture
     * for each texture id, we need to call _create_texture(), and
     * place them in the hashtable
     */

    CACHING_CLIENT(client)->super_dispatch.glGenTextures (client, n, server_textures);

    /* add textures to cache */
    for (i = 0; i < n; i++) {
        texture_t *tex = _create_texture (textures[i]);
        HashInsert (egl_state_get_texture_cache (state), textures[i], tex);
    }
}

static void
caching_client_glGenerateMipmap (void* client, GLenum target)
{
    INSTRUMENT();

    if (! (target == GL_TEXTURE_2D       || 
           target == GL_TEXTURE_CUBE_MAP
                                         || 
           target == GL_TEXTURE_3D_OES
                                      )) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glGenerateMipmap (client, target);
    caching_client_set_needs_get_error (CLIENT (client));
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

    CACHING_CLIENT(client)->super_dispatch.glGetActiveUniform (client, program, index, bufsize,
                                                               length, size, type, name);
    if (*length == 0)
        caching_client_set_needs_get_error (CLIENT (client));
}

static GLint
caching_client_glGetAttribLocation (void* client, GLuint program,
                                    const GLchar *name)
{
    INSTRUMENT();

    GLuint *location_pointer;
    GLint result;
    egl_state_t *state = client_get_current_state (CLIENT (client));
    GLuint *data;
    link_list_t *program_list = state->programs;
    program_t *saved_program;

    while (program_list) {
        saved_program = (program_t *)program_list->data;
        if (saved_program->id == program) {
            location_pointer = (GLuint *)HashLookup (saved_program->attrib_location_cache,
                                                     HashStr (name));

            if (location_pointer)
                return *location_pointer;

            result = CACHING_CLIENT(client)->super_dispatch.glGetAttribLocation (client, program, name);

            if (result == -1) {
                caching_client_set_needs_get_error (CLIENT (client));
                return -1;
            }

            data = (GLuint *)malloc (sizeof (GLuint));
            *data = result;
            HashInsert (saved_program->attrib_location_cache, HashStr(name), data);

            return result;
        }
        program_list = program_list->next; 
    }
    return -1;
}

static void
caching_client_glLinkProgram (void* client,
                              GLuint program)
{
    command_t *command = client_get_space_for_command (COMMAND_GLLINKPROGRAM);
    egl_state_t *state = client_get_current_state (CLIENT (client));
    link_list_t *program_list = state->programs;
    program_t *saved_program;

    /* need to check validity of program */
    while (program_list) {
        saved_program = (program_t *) program_list->data;
        if (saved_program->id == program) { 
            command_gllinkprogram_init (command,
                                program);

            client_run_command_async (command);
            return;
        }
        program_list = program_list->next;
    }

    caching_client_glSetError (client, GL_INVALID_VALUE);
}

static GLint
caching_client_glGetUniformLocation (void* client, GLuint program,
                                     const GLchar *name)
{
    INSTRUMENT();

    GLuint *location_pointer;
    GLint result;
    egl_state_t *state = client_get_current_state (CLIENT (client));
    GLuint *data;
    link_list_t *program_list = state->programs;
    program_t *saved_program;

    while (program_list) {
        saved_program = (program_t *)program_list->data;
        if (saved_program->id == program) {
            location_pointer = (GLuint *)HashLookup (saved_program->uniform_location_cache,
                                                     HashStr (name));

            if (location_pointer)
                return *location_pointer;

            result = CACHING_CLIENT(client)->super_dispatch.glGetUniformLocation (client, program, name);

            if (result == -1) {
                caching_client_set_needs_get_error (CLIENT (client));
                return -1;
            }

            data = (GLuint *)malloc (sizeof (GLuint));
            *data = result;
            HashInsert (saved_program->uniform_location_cache, HashStr(name), data);

            return result;
        }
        program_list = program_list->next;
    }
    return -1;
}

static void
caching_client_glGetBooleanv (void* client, GLenum pname, GLboolean *params)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    switch (pname) {
    case GL_BLEND:
        *params = state->blend;
        break;
    case GL_COLOR_WRITEMASK:
        memcpy (params, state->color_writemask, sizeof (GLboolean) * 4);
        break;
    case GL_CULL_FACE:
        *params = state->cull_face;
        break;
    case GL_DEPTH_TEST:
        *params = state->depth_test;
        break;
    case GL_DEPTH_WRITEMASK:
        *params = state->depth_writemask;
        break;
    case GL_DITHER:
        *params = state->dither;
        break;
    case GL_POLYGON_OFFSET_FILL:
        *params = state->polygon_offset_fill;
        break;
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
        *params = state->sample_alpha_to_coverage;
        break;
    case GL_SAMPLE_COVERAGE:
        *params = state->sample_coverage;
        break;
    case GL_SCISSOR_TEST:
        *params = state->scissor_test;
        break;
    case GL_SHADER_COMPILER:
        *params = state->shader_compiler;
        break;
    case GL_STENCIL_TEST:
        *params = state->stencil_test;
        break;
    default:
        CACHING_CLIENT(client)->super_dispatch.glGetBooleanv (client, pname, params);
        break;
    }
}

static void
caching_client_glGetFloatv (void* client, GLenum pname, GLfloat *params)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    switch (pname) {
    case GL_BLEND_COLOR:
        memcpy (params, state->blend_color, sizeof (GLfloat) * 4);
        break;
    case GL_BLEND_DST_ALPHA:
        *params = state->blend_dst[1];
        break;
    case GL_BLEND_DST_RGB:
        *params = state->blend_dst[0];
        break;
    case GL_BLEND_EQUATION_ALPHA:
        *params = state->blend_equation[1];
        break;
    case GL_BLEND_EQUATION_RGB:
        *params = state->blend_equation[0];
        break;
    case GL_BLEND_SRC_ALPHA:
        *params = state->blend_src[1];
        break;
    case GL_BLEND_SRC_RGB:
        *params = state->blend_src[0];
        break;
    case GL_COLOR_CLEAR_VALUE:
        memcpy (params, state->color_clear_value, sizeof (GLfloat) * 4);
        break;
    case GL_DEPTH_CLEAR_VALUE:
        *params = state->depth_clear_value;
        break;
    case GL_DEPTH_RANGE:
        memcpy (params, state->depth_range, sizeof (GLfloat) * 2);
        break;
    case GL_LINE_WIDTH:
        *params = state->line_width;
        break;
    case GL_POLYGON_OFFSET_FACTOR:
        *params = state->polygon_offset_factor;
        break; 
    default:
        CACHING_CLIENT(client)->super_dispatch.glGetFloatv (client, pname, params);
        break;
    }
}

static void
caching_client_glGetIntegerv (void* client,
                              GLenum pname,
                              GLint *params)
{
    vertex_attrib_list_t *attrib_list;
    vertex_attrib_t *attribs;
    int count;
    int i;

    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    attrib_list = &state->vertex_attribs;
    attribs = attrib_list->attribs;
    count = attrib_list->count;

    switch (pname) {
    case GL_ACTIVE_TEXTURE:
        *params = state->active_texture;
        break;
    case GL_CURRENT_PROGRAM:
        *params = state->current_program;
        break;
    case GL_DEPTH_CLEAR_VALUE:
        *params = state->depth_clear_value;
        break;
    case GL_DEPTH_FUNC:
        *params = state->depth_func;
        break;
    case GL_DEPTH_RANGE:
        params[0] = state->depth_range[0];
        params[1] = state->depth_range[1];
        break;
    case GL_GENERATE_MIPMAP_HINT:
        *params = state->generate_mipmap_hint;
        break;
    case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
        if (! state->max_combined_texture_image_units_queried) {

            CACHING_CLIENT(client)->super_dispatch.glGetIntegerv (client, pname, params);
            state->max_combined_texture_image_units_queried = true;
            state->max_combined_texture_image_units = *params;
        } else
            *params = state->max_combined_texture_image_units;
        break;
    case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
        if (! state->max_cube_map_texture_size_queried) {
            CACHING_CLIENT(client)->super_dispatch.glGetIntegerv (client, pname, params);
            state->max_cube_map_texture_size = *params;
            state->max_cube_map_texture_size_queried = true;
        } else
            *params = state->max_cube_map_texture_size;
    break;
    case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
        if (! state->max_fragment_uniform_vectors_queried) {
            CACHING_CLIENT(client)->super_dispatch.glGetIntegerv (client, pname, params);
            state->max_fragment_uniform_vectors = *params;
            state->max_fragment_uniform_vectors_queried = true;
        } else
            *params = state->max_fragment_uniform_vectors;
        break;
    case GL_MAX_RENDERBUFFER_SIZE:
        if (! state->max_renderbuffer_size_queried) {
            CACHING_CLIENT(client)->super_dispatch.glGetIntegerv (client, pname, params);
            state->max_renderbuffer_size = *params;
            state->max_renderbuffer_size_queried = true;
        } else
            *params = state->max_renderbuffer_size;
        break;
    case GL_MAX_TEXTURE_IMAGE_UNITS:
        if (! state->max_texture_image_units_queried) {
            CACHING_CLIENT(client)->super_dispatch.glGetIntegerv (client, pname, params);
            state->max_texture_image_units = *params;
            state->max_texture_image_units_queried = true;
        } else
            *params = state->max_texture_image_units;
        break;
    case GL_MAX_VARYING_VECTORS:
        if (! state->max_varying_vectors_queried) {
            CACHING_CLIENT(client)->super_dispatch.glGetIntegerv (client, pname, params);
            state->max_varying_vectors = *params;
            state->max_varying_vectors_queried = true;
        } else
            *params = state->max_varying_vectors;
        break;
    case GL_MAX_TEXTURE_SIZE:
        if (! state->max_texture_size_queried) {
            CACHING_CLIENT(client)->super_dispatch.glGetIntegerv (client, pname, params);
            state->max_texture_size = *params;
            state->max_texture_size_queried = true;
        } else
            *params = state->max_texture_size;
        break;
    case GL_MAX_VERTEX_ATTRIBS:
        if (! state->max_vertex_attribs_queried) {
            CACHING_CLIENT(client)->super_dispatch.glGetIntegerv (client, pname, params);
            state->max_vertex_attribs = *params;
            state->max_vertex_attribs_queried = true;
        } else
            *params = state->max_vertex_attribs;
        break;
    case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
        if (! state->max_vertex_texture_image_units_queried) {
            CACHING_CLIENT(client)->super_dispatch.glGetIntegerv (client, pname, params);
            state->max_vertex_texture_image_units = *params;
            state->max_vertex_texture_image_units_queried = true;
        } else
            *params = state->max_vertex_texture_image_units;
        break;
    case GL_MAX_VERTEX_UNIFORM_VECTORS:
        if (! state->max_vertex_uniform_vectors_queried) {
            CACHING_CLIENT(client)->super_dispatch.glGetIntegerv (client, pname, params);
            state->max_vertex_uniform_vectors = *params;
            state->max_vertex_uniform_vectors_queried = true;
        } else
            *params = state->max_vertex_uniform_vectors;
        break;
    case GL_POLYGON_OFFSET_UNITS:
        *params = state->polygon_offset_units;
        break;
    case GL_SCISSOR_BOX:
        memcpy (params, state->scissor_box, sizeof (GLint) * 4);
        break;
    case GL_STENCIL_BACK_FAIL:
        *params = state->stencil_back_fail;
        break;
    case GL_STENCIL_BACK_FUNC:
        *params = state->stencil_back_func;
        break;
    case GL_STENCIL_BACK_PASS_DEPTH_FAIL:
        *params = state->stencil_back_pass_depth_fail;
        break;
    case GL_STENCIL_BACK_PASS_DEPTH_PASS:
        *params = state->stencil_back_pass_depth_pass;
        break;
    case GL_STENCIL_BACK_REF:
        *params = state->stencil_ref;
        break;
    case GL_STENCIL_BACK_VALUE_MASK:
        *params = state->stencil_value_mask;
        break;
    case GL_STENCIL_CLEAR_VALUE:
        *params = state->stencil_clear_value;
        break;
    case GL_STENCIL_FAIL:
        *params = state->stencil_fail;
        break;
    case GL_STENCIL_FUNC:
        *params = state->stencil_func;
        break;
    case GL_STENCIL_PASS_DEPTH_FAIL:
        *params = state->stencil_pass_depth_fail;
        break;
    case GL_STENCIL_PASS_DEPTH_PASS:
        *params = state->stencil_pass_depth_pass;
        break;
    case GL_STENCIL_REF:
        *params = state->stencil_ref;
        break;
    case GL_STENCIL_VALUE_MASK:
        *params = state->stencil_value_mask;
        break;
    case GL_STENCIL_WRITEMASK:
        *params = state->stencil_writemask;
        break;
    case GL_STENCIL_BACK_WRITEMASK:
        *params = state->stencil_back_writemask;
        break;
    case GL_VIEWPORT:
        memcpy (params, state->viewport, sizeof (GLint) * 4);
        break;
    default:
        CACHING_CLIENT(client)->super_dispatch.glGetIntegerv (client, pname, params);
        caching_client_set_needs_get_error (CLIENT (client));
        break;
    }

    if (pname == GL_ARRAY_BUFFER_BINDING) {
        state->array_buffer_binding = *params;
        /* update client state */
        for (i = 0; i < count; i++) {
            attribs[i].array_buffer_binding = *params;
        }
    }
    else if (pname == GL_ELEMENT_ARRAY_BUFFER_BINDING)
        state->array_buffer_binding = *params;
    else if (pname == GL_VERTEX_ARRAY_BINDING_OES)
        state->vertex_array_binding = *params;
}

static GLenum
caching_client_glGetError (void* client)
{
    GLenum error = GL_INVALID_OPERATION;

    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
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

    INSTRUMENT();

    if (! (name == GL_VENDOR                   || 
           name == GL_RENDERER                 ||
           name == GL_SHADING_LANGUAGE_VERSION ||
           name == GL_EXTENSIONS               ||
           name == GL_VERSION)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return NULL;
    }

    result = CACHING_CLIENT(client)->super_dispatch.glGetString (client, name);

    if (result == 0)
        caching_client_set_needs_get_error (CLIENT (client));

    return result;
}

static void
caching_client_glGetTexParameteriv (void* client, GLenum target, GLenum pname, 
                                     GLint *params)
{
    int active_texture_index;
    int target_index;

    INSTRUMENT();

    if (! (target == GL_TEXTURE_2D       ||
           target == GL_TEXTURE_CUBE_MAP ||
           target == GL_TEXTURE_3D_OES)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    if (! (pname == GL_TEXTURE_MAG_FILTER ||
           pname == GL_TEXTURE_MIN_FILTER ||
           pname == GL_TEXTURE_WRAP_S     ||
           pname == GL_TEXTURE_WRAP_T     ||
           pname == GL_TEXTURE_WRAP_R_OES)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    egl_state_t *state = client_get_current_state (CLIENT (client));
    active_texture_index = state->active_texture - GL_TEXTURE0;
    if (target == GL_TEXTURE_2D)
        target_index = 0;
    else if (target == GL_TEXTURE_CUBE_MAP)
        target_index = 1;
    else
        target_index = 2;

    if (pname == GL_TEXTURE_MAG_FILTER)
        *params = state->texture_mag_filter[active_texture_index][target_index];
    else if (pname == GL_TEXTURE_MIN_FILTER)
        *params = state->texture_min_filter[active_texture_index][target_index];
    else if (pname == GL_TEXTURE_WRAP_S)
        *params = state->texture_wrap_s[active_texture_index][target_index];
    else if (pname == GL_TEXTURE_WRAP_T)
        *params = state->texture_wrap_t[active_texture_index][target_index];
    else if (pname == GL_TEXTURE_WRAP_R_OES)
        *params = state->texture_3d_wrap_r[active_texture_index];
}

static void
caching_client_glGetTexParameterfv (void* client, GLenum target, GLenum pname, GLfloat *params)
{
    GLint paramsi;

    caching_client_glGetTexParameteriv (client, target, pname, &paramsi);
    *params = paramsi;
}

static void
caching_client_glGetUniformfv (void* client, GLuint program,
                               GLint location,  GLfloat *params)
{
    GLfloat original_params = *params;

    INSTRUMENT();

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
    attrib_list = &state->vertex_attribs;
    attribs = attrib_list->attribs;
    count = attrib_list->count;

    if (! (pname == GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING ||
           pname == GL_VERTEX_ATTRIB_ARRAY_ENABLED        ||
           pname == GL_VERTEX_ATTRIB_ARRAY_SIZE           ||
           pname == GL_VERTEX_ATTRIB_ARRAY_STRIDE         ||
           pname == GL_VERTEX_ATTRIB_ARRAY_TYPE           ||
           pname == GL_VERTEX_ATTRIB_ARRAY_NORMALIZED     ||
           pname == GL_CURRENT_VERTEX_ATTRIB)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    /* check index is too large */
    if (caching_client_does_index_overflow (client, index)) {
        return;
    }

    /* we cannot use client state */
    if (state->vertex_array_binding) {
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
    attrib_list = &state->vertex_attribs;
    attribs = attrib_list->attribs;
    count = attrib_list->count;

    if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    /* XXX: check index validity */
    if (caching_client_does_index_overflow (client, index)) {
        return;
    }

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
    if (target == GL_GENERATE_MIPMAP_HINT &&
        state->generate_mipmap_hint == mode)
        return;

    if ( !(mode == GL_FASTEST ||
           mode == GL_NICEST  ||
           mode == GL_DONT_CARE)) {
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
    texture_t *tex = (texture_t *)HashLookup (egl_state_get_texture_cache (state), texture);
    if (! tex)
        return GL_FALSE;

    return GL_TRUE;
}

static void
caching_client_glPixelStorei (void* client, GLenum pname, GLint param)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if ((pname == GL_PACK_ALIGNMENT                &&
         state->pack_alignment == param) ||
        (pname == GL_UNPACK_ALIGNMENT              &&
         state->unpack_alignment == param))
        return;

    if (! (param == 1 ||
           param == 2 ||
           param == 4 ||
           param == 8)) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }
    else if (! (pname == GL_PACK_ALIGNMENT ||
                pname == GL_UNPACK_ALIGNMENT)) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    if (pname == GL_PACK_ALIGNMENT)
       state->pack_alignment = param;
    else
       state->unpack_alignment = param;

    CACHING_CLIENT(client)->super_dispatch.glPixelStorei (client, pname, param);
}

static void
caching_client_glPolygonOffset (void* client, GLfloat factor, GLfloat units)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
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
    if (! (face == GL_FRONT ||
           face == GL_BACK ||
           face == GL_FRONT_AND_BACK)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    if (! (func == GL_NEVER ||
                func == GL_LESS ||
                func == GL_LEQUAL ||
                func == GL_GREATER ||
                func == GL_GEQUAL ||
                func == GL_EQUAL ||
                func == GL_NOTEQUAL ||
                func == GL_ALWAYS)) {
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
    caching_client_glStencilFuncSeparate (client, GL_FRONT_AND_BACK, func, ref, mask);
}

static void
caching_client_glStencilMaskSeparate (void* client, GLenum face, GLuint mask)
{
    bool needs_call = false;

    INSTRUMENT();

    if (! (face == GL_FRONT         ||
           face == GL_BACK          ||
           face == GL_FRONT_AND_BACK)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    egl_state_t *state = client_get_current_state (CLIENT (client));
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
    caching_client_glStencilMaskSeparate (client, GL_FRONT_AND_BACK, mask);
}

static void
caching_client_glStencilOpSeparate (void* client, GLenum face, GLenum sfail, 
                                     GLenum dpfail, GLenum dppass)
{
    bool needs_call = false;

    INSTRUMENT();

    if (! (face == GL_FRONT         ||
           face == GL_BACK          ||
           face == GL_FRONT_AND_BACK)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }
    else if (! (sfail == GL_KEEP       ||
                sfail == GL_ZERO       ||
                sfail == GL_REPLACE    ||
                sfail == GL_INCR       ||
                sfail == GL_INCR_WRAP  ||
                sfail == GL_DECR       ||
                sfail == GL_DECR_WRAP  ||
                sfail == GL_INVERT)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }
    else if (! (dpfail == GL_KEEP      ||
                dpfail == GL_ZERO      ||
                dpfail == GL_REPLACE   ||
                dpfail == GL_INCR      ||
                dpfail == GL_INCR_WRAP ||
                dpfail == GL_DECR      ||
                dpfail == GL_DECR_WRAP ||
                dpfail == GL_INVERT)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }
    else if (! (dppass == GL_KEEP      ||
                dppass == GL_ZERO      ||
                dppass == GL_REPLACE   ||
                dppass == GL_INCR      ||
                dppass == GL_INCR_WRAP ||
                dppass == GL_DECR      ||
                dppass == GL_DECR_WRAP ||
                dppass == GL_INVERT)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    egl_state_t *state = client_get_current_state (CLIENT (client));
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

    caching_client_glStencilOpSeparate (client, GL_FRONT_AND_BACK, sfail, dpfail, dppass);
}

static void
caching_client_glTexParameteri (void* client, GLenum target, GLenum pname, GLint param)
{
    int active_texture_index;
    int target_index;

    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    active_texture_index = state->active_texture - GL_TEXTURE0;

    if (! (target == GL_TEXTURE_2D || 
           target == GL_TEXTURE_CUBE_MAP || 
           target == GL_TEXTURE_3D_OES)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    if (target == GL_TEXTURE_2D)
        target_index = 0;
    else if (target == GL_TEXTURE_CUBE_MAP)
        target_index = 1;
    else
        target_index = 2;

    if (pname == GL_TEXTURE_MAG_FILTER) {
        if (state->texture_mag_filter[active_texture_index][target_index] != param) {
            state->texture_mag_filter[active_texture_index][target_index] = param;
        }
        else
            return;
    }
    else if (pname == GL_TEXTURE_MIN_FILTER) {
        if (state->texture_min_filter[active_texture_index][target_index] != param) {
            state->texture_min_filter[active_texture_index][target_index] = param;
        }
        else
            return;
    }
    else if (pname == GL_TEXTURE_WRAP_S) {
        if (state->texture_wrap_s[active_texture_index][target_index] != param) {
            state->texture_wrap_s[active_texture_index][target_index] = param;
        }
        else
            return;
    }
    else if (pname == GL_TEXTURE_WRAP_T) {
        if (state->texture_wrap_t[active_texture_index][target_index] != param) {
            state->texture_wrap_t[active_texture_index][target_index] = param;
        }
        else
            return;
    }
    else if (pname == GL_TEXTURE_WRAP_R_OES) {
        if (state->texture_3d_wrap_r[active_texture_index] != param) {
            state->texture_3d_wrap_r[active_texture_index] = param;
        }
        else
            return;
    }

    if (! (pname == GL_TEXTURE_MAG_FILTER ||
           pname == GL_TEXTURE_MIN_FILTER ||
           pname == GL_TEXTURE_WRAP_S     ||
           pname == GL_TEXTURE_WRAP_T
                                          || 
           pname == GL_TEXTURE_WRAP_R_OES
                                         )) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    if (pname == GL_TEXTURE_MAG_FILTER &&
        ! (param == GL_NEAREST ||
           param == GL_LINEAR ||
           param == GL_NEAREST_MIPMAP_NEAREST ||
           param == GL_LINEAR_MIPMAP_NEAREST  ||
           param == GL_NEAREST_MIPMAP_LINEAR  ||
           param == GL_LINEAR_MIPMAP_LINEAR)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }
    else if (pname == GL_TEXTURE_MIN_FILTER &&
             ! (param == GL_NEAREST ||
                param == GL_LINEAR)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }
    else if ((pname == GL_TEXTURE_WRAP_S ||
              pname == GL_TEXTURE_WRAP_T
                                         || 
              pname == GL_TEXTURE_WRAP_R_OES
                                            ) &&
             ! (param == GL_CLAMP_TO_EDGE   ||
                param == GL_MIRRORED_REPEAT ||
                param == GL_REPEAT)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }
    CACHING_CLIENT(client)->super_dispatch.glTexParameteri (client, target, pname, param);
}

static void
caching_client_glTexParameterf (void* client, GLenum target, GLenum pname, GLfloat param)
{
    GLint parami = param;

    INSTRUMENT();
    caching_client_glTexParameteri (client, target, pname, parami);
}

static void
caching_client_glTexImage2D (void* client, GLenum target, GLint level,
                             GLint internalformat, GLsizei width,
                             GLsizei height, GLint border, GLenum format,
                             GLenum type, const void *pixels)
{
    texture_t *tex;
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
     
    /* FIXME: we need more checks on max width/height and level */ 
    if (target == GL_TEXTURE_2D)
        tex_id = state->texture_binding[0];
    else
        tex_id = state->texture_binding[1];

    tex = (texture_t *)HashLookup (egl_state_get_texture_cache (state), tex_id);
    if (! tex)
        caching_client_set_needs_get_error (CLIENT (client));
    else {
        tex->internal_format = internalformat;
        tex->width = width;
        tex->height = height;
        tex->data_type = type;
    }

    CACHING_CLIENT(client)->super_dispatch.glTexImage2D (client, target, level, internalformat,
                                                         width, height, border, format, type, pixels);
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
    texture_t *tex;
    GLuint tex_id;
    egl_state_t *state = client_get_current_state (CLIENT (client));

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
    
    if (target == GL_TEXTURE_2D)
        tex_id = state->texture_binding[0];
    else
        tex_id = state->texture_binding[1];

    tex = (texture_t *)HashLookup (egl_state_get_texture_cache (state), tex_id);
    if (! tex)
        caching_client_glSetError (client, GL_INVALID_OPERATION);
    else {
        if (tex->internal_format != format) {
            caching_client_glSetError (client, GL_INVALID_OPERATION);
            return;
        }
        if (xoffset + width > tex->width ||
            yoffset + height > tex->height) {        
            caching_client_glSetError (client, GL_INVALID_VALUE);
            return;
        }
    }

    /* FIXME: we need to check level */

    CACHING_CLIENT(client)->super_dispatch.glTexSubImage2D (client, target, level, xoffset, yoffset,
                                                            width, height, format, type, pixels);
}

static void
caching_client_glUniformMatrix2fv (void* client,
                                   GLint location,
                                   GLsizei count,
                                   GLboolean transpose,
                                   const GLfloat *value)
{
    INSTRUMENT();

    if (count < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glUniformMatrix2fv (client, location, count, transpose, value);
}

static void
caching_client_glUniformMatrix3fv (void* client,
                                   GLint location,
                                   GLsizei count,
                                   GLboolean transpose,
                                   const GLfloat *value)
{
    INSTRUMENT();

    if (count < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }
    CACHING_CLIENT(client)->super_dispatch.glUniformMatrix3fv (client, location, count, transpose, value);
}

static void
caching_client_glUniformMatrix4fv (void* client,
                                   GLint location,
                                   GLsizei count,
                                   GLboolean transpose,
                                   const GLfloat *value)
{
    INSTRUMENT();

    if (count < 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return; 
    }
    CACHING_CLIENT(client)->super_dispatch.glUniformMatrix4fv (client, location, count, transpose, value);
}

static void
caching_client_glUseProgram (void* client, GLuint program)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    link_list_t *program_list = state->programs;
    program_t *saved_program;
    bool found = false;
    link_list_t *saved_program_list = NULL;
    
    if (state->current_program == program)
        return;
    if (program < 0) {
        caching_client_glSetError (client, GL_INVALID_OPERATION);
        return;
    }
    /* this maybe not right because this program may be invalid
     * object, we save here to save time in glGetError() */
    while (program_list) {
        saved_program = (program_t *) program_list->data;
        if (saved_program->id == program) { 
            found = true;
            break;
        }
        program_list = program_list->next;
    }
    if (found) {
        CACHING_CLIENT(client)->super_dispatch.glUseProgram (client, program);

        /* this maybe not right because this program may be invalid
         * object, we save here to save time in glGetError() */

        program_list = state->programs;
        while (program_list) {
            saved_program = (program_t *) program_list->data;
            if (saved_program->id == state->current_program &&
                saved_program->mark_for_deletion) { 
                found = true;
                saved_program_list = program_list;
                break;
            }
            program_list = program_list->next;
        }

        if (saved_program_list) {
            program_list = saved_program_list->prev;
            if (program_list)
                program_list->next = saved_program_list->next;
    
            program_list = saved_program_list->next;
            if (program_list)
                program_list->prev = saved_program_list->prev;

            DeleteHashTable (saved_program->attrib_location_cache);
            DeleteHashTable (saved_program->uniform_location_cache);
            free (saved_program);
            free (saved_program_list);
        }
        state->current_program = program;
        return;
    }
    caching_client_glSetError (client, GL_INVALID_OPERATION);
}

static void
caching_client_glVertexAttrib1f (void* client, GLuint index, GLfloat v0)
{

    INSTRUMENT();

    if (caching_client_does_index_overflow (client, index)) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glVertexAttrib1f (client, index, v0);
}

static void
caching_client_glVertexAttrib2f (void* client, GLuint index, GLfloat v0, GLfloat v1)
{
    INSTRUMENT();

    if (caching_client_does_index_overflow (client, index)) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glVertexAttrib2f (client, index, v0, v1);
}

static void
caching_client_glVertexAttrib3f (void* client, GLuint index, GLfloat v0, 
                                 GLfloat v1, GLfloat v2)
{
    INSTRUMENT();

    if (caching_client_does_index_overflow (client, index)) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glVertexAttrib3f (client, index, v0, v1, v2);
}

static void
caching_client_glVertexAttrib4f (void* client, GLuint index, GLfloat v0, GLfloat v1, 
                                 GLfloat v2, GLfloat v3)
{
    INSTRUMENT();

    if (caching_client_does_index_overflow (client, index)) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glVertexAttrib4f (client, index, v0, v1, v2, v3);
}

static void
caching_client_glVertexAttrib1fv (void* client, GLuint index, const GLfloat *v)
{
    INSTRUMENT();

    if (caching_client_does_index_overflow (client, index)) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }
    CACHING_CLIENT(client)->super_dispatch.glVertexAttrib1fv (client, index, v);
}

static void
caching_client_glVertexAttrib2fv (void* client, GLuint index, const GLfloat *v)
{
    INSTRUMENT();

    if (caching_client_does_index_overflow (client, index)) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }
    CACHING_CLIENT(client)->super_dispatch.glVertexAttrib2fv (client, index, v);
}

static void
caching_client_glVertexAttrib3fv (void* client, GLuint index, const GLfloat *v)
{
    INSTRUMENT();

    if (caching_client_does_index_overflow (client, index)) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }
    CACHING_CLIENT(client)->super_dispatch.glVertexAttrib3fv (client, index, v);
}

static void
caching_client_glVertexAttrib4fv (void* client, GLuint index, const GLfloat *v)
{
    INSTRUMENT();

    if (caching_client_does_index_overflow (client, index)) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }
    CACHING_CLIENT(client)->super_dispatch.glVertexAttrib4fv (client, index, v);
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
    attrib_list = &state->vertex_attribs;
    attribs = attrib_list->attribs;
    count = attrib_list->count;

    if (! (type == GL_BYTE                 ||
           type == GL_UNSIGNED_BYTE        ||
           type == GL_SHORT                ||
           type == GL_UNSIGNED_SHORT       ||
           type == GL_FLOAT)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }
    else if (size > 4 || size < 1 || stride < 0) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    /* check max_vertex_attribs */
    if (caching_client_does_index_overflow (client, index)) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

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
        /* FIXME: Do we need set this flag? */
        caching_client_set_needs_get_error (CLIENT (client));
    }

    if (found_index != -1) {
        attribs[found_index].size = size;
        attribs[found_index].stride = stride;
        attribs[found_index].type = type;
        attribs[found_index].array_normalized = normalized;
        attribs[found_index].pointer = (GLvoid *)pointer;
        attribs[found_index].array_buffer_binding = bound_buffer;

        if (! bound_buffer && attribs[found_index].array_enabled) {
            _set_vertex_pointers (attrib_list, &attribs[found_index]);
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
        memset (attribs[i].current_attrib, 0, sizeof (GLfloat) * 4);
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
        memset (new_attribs[count].current_attrib, 0, sizeof (GLfloat) * 4);

        attrib_list->attribs = new_attribs;
        attrib_list->count ++;
    }
}

static void
caching_client_glViewport (void* client, GLint x, GLint y, GLsizei width, GLsizei height)
{
    INSTRUMENT();

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (state->viewport[0] == x     &&
        state->viewport[1] == y     &&
        state->viewport[2] == width &&
        state->viewport[3] == height)
        return;

    if (x < 0 || y < 0) {
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
    caching_client_set_needs_get_error (CLIENT (client));
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
    else if (! (target == GL_ARRAY_BUFFER ||
                target == GL_ELEMENT_ARRAY_BUFFER)) {
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

    if (! (target == GL_ARRAY_BUFFER ||
           target == GL_ELEMENT_ARRAY_BUFFER)) {
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

    if (! (target == GL_ARRAY_BUFFER ||
           target == GL_ELEMENT_ARRAY_BUFFER)) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glGetBufferPointervOES (client, target, pname, params);
    caching_client_set_needs_get_error (CLIENT (client));
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
    INSTRUMENT();

    if (target != GL_FRAMEBUFFER) {
        caching_client_glSetError (client, GL_INVALID_ENUM);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glFramebufferTexture3DOES (client, target, attachment,
                                                                      textarget, texture, level, zoffset);
    caching_client_set_needs_get_error (CLIENT (client));
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

    egl_state_t *state = client_get_current_state (CLIENT (client));
    if (state->vertex_array_binding == array)
        return;

    CACHING_CLIENT(client)->super_dispatch.glBindVertexArrayOES (client, array);
    caching_client_set_needs_get_error (CLIENT (client));

    /* FIXME: should we save this ? */
    state->vertex_array_binding = array;
}

static void
caching_client_glDeleteVertexArraysOES (void* client, GLsizei n, const GLuint *arrays)
{
    INSTRUMENT();

    if (n <= 0) {
        caching_client_glSetError (client, GL_INVALID_VALUE);
        return;
    }

    CACHING_CLIENT(client)->super_dispatch.glDeleteVertexArraysOES (client, n, arrays);

    /* matching vertex_array_binding ? */
    egl_state_t *state = client_get_current_state (CLIENT (client));
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

    result = CACHING_CLIENT(client)->super_dispatch.glIsVertexArrayOES (client, array);

    egl_state_t *state = client_get_current_state (CLIENT (client));
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
        if (! (attachments[i] == GL_COLOR_ATTACHMENT0  ||
               attachments[i] == GL_DEPTH_ATTACHMENT   ||
               attachments[i] == GL_STENCIL_ATTACHMENT ||
               attachments[i] == GL_COLOR_EXT          ||
               attachments[i] == GL_DEPTH_EXT          ||
               attachments[i] == GL_STENCIL_EXT)) {
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
    INSTRUMENT();

    if (target != GL_FRAMEBUFFER) {
       caching_client_glSetError (client, GL_INVALID_ENUM);
       return;
    }

    CACHING_CLIENT(client)->super_dispatch.glFramebufferTexture2DMultisampleEXT (
        client, target, attachment, textarget, texture, level, samples);
    caching_client_set_needs_get_error (CLIENT (client));
}

static void
caching_client_glFramebufferTexture2DMultisampleIMG (void* client, GLenum target, 
                                                     GLenum attachment,
                                                     GLenum textarget, 
                                                     GLuint texture,
                                                     GLint level, GLsizei samples)
{
    INSTRUMENT();

    if (target != GL_FRAMEBUFFER) {
       caching_client_glSetError (client, GL_INVALID_ENUM);
       return;
    }

    CACHING_CLIENT(client)->super_dispatch.glFramebufferTexture2DMultisampleIMG (
        client, target, attachment, textarget, texture, level, samples);
    caching_client_set_needs_get_error (CLIENT (client));
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

    egl_state_t *state = find_state_with_display_and_context (dpy, ctx, false);
    if (!state) {
        mutex_unlock (cached_gl_states_mutex);
        return EGL_TRUE;
    }

    if (! state->active)
        _caching_client_destroy_state (client, state);
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
    CLIENT(client)->last_1k_index = 0;
    CLIENT(client)->last_2k_index = 0;
    CLIENT(client)->last_4k_index = 0;
    CLIENT(client)->last_8k_index = 0;
    CLIENT(client)->last_16k_index = 0;
    CLIENT(client)->last_32k_index = 0;

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
    if (share_context == EGL_NO_CONTEXT || result == EGL_NO_CONTEXT)
        return result;

    egl_state_t *new_state = _caching_client_get_or_create_state (dpy, result);
    new_state->share_context = _caching_client_get_or_create_state (dpy, share_context);
    new_state->share_context->contexts_sharing++;
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
    bool display_and_context_match = current_state &&
                                     current_state->display == display &&
                                     current_state->context == ctx;
    if (display_and_context_match &&
        current_state->drawable == draw && current_state->readable == read) {
        return EGL_TRUE;
    }

    /* TODO: This should really go somewhere else. */
    if (!display_and_context_match && current_state)
        current_state->active = false;

    /* We aren't switching to none and we aren't switch to the same context
     * with different surfaces. We can do this asynchronous if we know that
     * the last time we used this context we used the same surfaces. */
    egl_state_t *matching_state = NULL;
    if (!display_and_context_match && !switching_to_none) {
        matching_state = find_state_with_display_and_context (display, ctx, true);

        /* We cannot activate this context asynchronously if we are switching
         * read or write surfaces, if it's an active context or if we have
         * queued destroying its display. */
        if (matching_state &&
            ( matching_state->drawable != draw ||
              matching_state->readable != read ||
              matching_state->active ||
              matching_state->destroy_dpy)) {
            matching_state = NULL;
         }
    }

    /* If we are switching to none or we have previously activated this context
     * with this pair of surfaces, we can safely do this operation asynchronously.
     * We know it will succeed */
    if (switching_to_none || matching_state != NULL) {
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

/* we specify those passthrough GL APIs that needs to set need_get_error */
static void
caching_client_post_hook(client_t *client,
                         command_t *command)
{
    switch (command->type) {
    case COMMAND_GLATTACHSHADER:
    case COMMAND_GLBINDATTRIBLOCATION:
    case COMMAND_GLBUFFERDATA:
    case COMMAND_GLBUFFERSUBDATA:
    case COMMAND_GLCOMPILESHADER:
    case COMMAND_GLCOMPRESSEDTEXIMAGE2D:
    case COMMAND_GLCOMPRESSEDTEXSUBIMAGE2D:
    case COMMAND_GLCOPYTEXIMAGE2D:
    case COMMAND_GLCOPYTEXSUBIMAGE2D:
    case COMMAND_GLDELETEPROGRAM:
    case COMMAND_GLDELETESHADER:
    case COMMAND_GLDETACHSHADER:
    case COMMAND_GLGETBUFFERPARAMETERIV:
    case COMMAND_GLGETPROGRAMINFOLOG:
    case COMMAND_GLGETSHADERIV:
    case COMMAND_GLGETSHADERPRECISIONFORMAT:
    case COMMAND_GLGETSHADERINFOLOG:
    case COMMAND_GLGETSHADERSOURCE:
    case COMMAND_GLLINKPROGRAM:
    case COMMAND_GLVALIDATEPROGRAM:
    case COMMAND_GLREADPIXELS:
    case COMMAND_GLRELEASESHADERCOMPILER:
    case COMMAND_GLRENDERBUFFERSTORAGE:
    case COMMAND_GLSHADERBINARY:
    case COMMAND_GLSHADERSOURCE:
    case COMMAND_GLEGLIMAGETARGETRENDERBUFFERSTORAGEOES:
    case COMMAND_GLGETPROGRAMBINARYOES:
    case COMMAND_GLPROGRAMBINARYOES:
    case COMMAND_GLGETBUFFERPOINTERVOES:
    case COMMAND_GLTEXIMAGE3DOES:
    case COMMAND_GLTEXSUBIMAGE3DOES:
    case COMMAND_GLCOPYTEXSUBIMAGE3DOES:
    case COMMAND_GLCOMPRESSEDTEXIMAGE3DOES:
    case COMMAND_GLCOMPRESSEDTEXSUBIMAGE3DOES:
    case COMMAND_GLFRAMEBUFFERTEXTURE3DOES:
    case COMMAND_GLBEGINPERFMONITORAMD:
    case COMMAND_GLGETPERFMONITORGROUPSAMD:
    case COMMAND_GLGETPERFMONITORCOUNTERSAMD:
    case COMMAND_GLGETPERFMONITORGROUPSTRINGAMD:
    case COMMAND_GLGETPERFMONITORCOUNTERSTRINGAMD:
    case COMMAND_GLGETPERFMONITORCOUNTERINFOAMD:
    case COMMAND_GLGENPERFMONITORSAMD:
    case COMMAND_GLENDPERFMONITORAMD:
    case COMMAND_GLDELETEPERFMONITORSAMD:
    case COMMAND_GLSELECTPERFMONITORCOUNTERSAMD:
    case COMMAND_GLGETPERFMONITORCOUNTERDATAAMD:
    case COMMAND_GLBLITFRAMEBUFFERANGLE:
    case COMMAND_GLRENDERBUFFERSTORAGEMULTISAMPLEANGLE:
    case COMMAND_GLRENDERBUFFERSTORAGEMULTISAMPLEAPPLE:
    case COMMAND_GLRESOLVEMULTISAMPLEFRAMEBUFFERAPPLE:
    case COMMAND_GLRENDERBUFFERSTORAGEMULTISAMPLEEXT:
    case COMMAND_GLRENDERBUFFERSTORAGEMULTISAMPLEIMG:
    case COMMAND_GLSETFENCENV:
    case COMMAND_GLFINISHFENCENV:
    case COMMAND_GLCOVERAGEMASKNV:
    case COMMAND_GLGETDRIVERCONTROLSQCOM:
    case COMMAND_GLENABLEDRIVERCONTROLQCOM:
    case COMMAND_GLDISABLEDRIVERCONTROLQCOM:
    case COMMAND_GLEXTTEXOBJECTSTATEOVERRIDEIQCOM:
    case COMMAND_GLGETDRIVERCONTROLSTRINGQCOM:
    case COMMAND_GLEXTGETTEXLEVELPARAMETERIVQCOM:
    case COMMAND_GLEXTGETTEXSUBIMAGEQCOM:
    case COMMAND_GLEXTGETBUFFERPOINTERVQCOM:
    case COMMAND_GLEXTISPROGRAMBINARYQCOM:
    case COMMAND_GLEXTGETPROGRAMBINARYSOURCEQCOM:
    case COMMAND_GLSTARTTILINGQCOM:
    case COMMAND_GLENDTILINGQCOM:
        caching_client_set_needs_get_error (CLIENT (client));
        break;
    default:
        break;
    }
}

static void
caching_client_init (caching_client_t *client)
{
    client_init (&client->super);
    client->super_dispatch = client->super.dispatch;

    /* Initialize the cached GL states. */
    mutex_lock (cached_gl_states_mutex);
    cached_gl_states ();
    mutex_unlock (cached_gl_states_mutex);

    client->super.post_hook = caching_client_post_hook;

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
