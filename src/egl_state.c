#include "config.h"
#include "egl_state.h"
#include "name_handler.h"
#include <stdlib.h>
#include <string.h>

void
egl_state_init (egl_state_t *state,
                EGLDisplay display,
                EGLContext context)
{
    state->context = context;
    state->display = display;
    state->drawable = EGL_NO_SURFACE;
    state->readable = EGL_NO_SURFACE;

    state->vendor_string = NULL;
    state->renderer_string = NULL;
    state->version_string = NULL;
    state->shading_language_version_string = NULL;
    state->extensions_string = NULL;

    state->share_context = NULL;
    state->contexts_sharing = 0;

    state->active = false;

    state->destroy_dpy = false;
    state->destroy_ctx = false;
    state->destroy_draw = false;
    state->destroy_read = false;

    state->vertex_attribs.count = 0;
    state->vertex_attribs.attribs = state->vertex_attribs.embedded_attribs;
    state->vertex_attribs.first_index_pointer = 0;
    state->vertex_attribs.last_index_pointer = 0;

    state->max_combined_texture_image_units = 8;
    state->max_vertex_attribs_queried = false;
    state->max_vertex_attribs = 8;
    state->max_cube_map_texture_size = 16;
    state->max_fragment_uniform_vectors = 16;
    state->max_fragment_uniform_vectors_queried = false;
    state->max_renderbuffer_size = 1;
    state->max_renderbuffer_size_queried = false;
    state->max_texture_image_units = 8;
    state->max_texture_image_units_queried = false;
    state->max_texture_size = 64;
    state->max_texture_size_queried = false;
    state->max_varying_vectors = 8;
    state->max_vertex_uniform_vectors = 128;
    state->max_vertex_texture_image_units = 0;

    state->error = GL_NO_ERROR;
    state->need_get_error = false;

    /* We add a head to the list so we can get a reference. */
    state->programs = NULL;

    state->active_texture = GL_TEXTURE0;
    state->array_buffer_binding = 0;
    state->vertex_array_binding = 0;

    state->blend = GL_FALSE;

    int i;
    for (i = 0; i < 4; i++) {
        state->blend_color[i] = GL_ZERO;
        state->blend_dst[i] = GL_ZERO;
        state->blend_src[i] = GL_ONE;
    }

    state->blend_equation[0] = state->blend_equation[1] = GL_FUNC_ADD;

    memset (state->color_clear_value, 0, sizeof (GLfloat) * 4);
    for (i = 0; i < 4; i++)
        state->color_writemask[i] = GL_TRUE;

    state->cull_face = GL_FALSE;
    state->cull_face_mode = GL_BACK;

    state->current_program = 0;

    state->depth_clear_value = 1;
    state->depth_func = GL_LESS;
    state->depth_range[0] = 0; state->depth_range[1] = 1;
    state->depth_test = GL_FALSE;
    state->depth_writemask = GL_TRUE;

    state->dither = GL_TRUE;

    state->element_array_buffer_binding = 0;
    state->framebuffer_binding = 0;
    state->renderbuffer_binding = 0;
    
    state->front_face = GL_CCW;
   
    state->generate_mipmap_hint = GL_DONT_CARE;

    state->line_width = 1;
    
    state->pack_alignment = 4; 
    state->unpack_alignment = 4;

    state->polygon_offset_factor = 0;
    state->polygon_offset_fill = GL_FALSE;
    state->polygon_offset_units = 0;

    state->sample_alpha_to_coverage = 0;
    state->sample_coverage = GL_FALSE;

    memset (state->scissor_box, 0, sizeof (GLint) * 4);
    state->scissor_test = GL_FALSE;

    /* XXX: should we set this */
    state->shader_compiler = GL_TRUE; 

    state->stencil_back_fail = GL_KEEP;
    state->stencil_back_func = GL_ALWAYS;
    state->stencil_back_pass_depth_fail = GL_KEEP;
    state->stencil_back_pass_depth_pass = GL_KEEP;
    state->stencil_back_ref = 0;
    memset (&state->stencil_back_value_mask, 1, sizeof (GLint));
    state->stencil_clear_value = 0;
    state->stencil_fail = GL_KEEP;
    state->stencil_func = GL_ALWAYS;
    state->stencil_pass_depth_fail = GL_KEEP;
    state->stencil_pass_depth_pass = GL_KEEP;
    state->stencil_ref = 0;
    state->stencil_test = GL_FALSE;
    memset (&state->stencil_value_mask, 1, sizeof (GLint));
    memset (&state->stencil_writemask, 1, sizeof (GLint));
    memset (&state->stencil_back_writemask, 1, sizeof (GLint));

    memset (state->texture_binding, 0, sizeof (GLint) * 2);

    memset (state->viewport, 0, sizeof (GLint) * 4);

    for (i = 0; i < 32; i++) {
        int j;
        for (j = 0; j < 3; j++) {
            state->texture_mag_filter[i][j] = GL_LINEAR;
            state->texture_mag_filter[i][j] = GL_NEAREST_MIPMAP_LINEAR;
            state->texture_min_filter[i][j] = GL_LINEAR;
            state->texture_min_filter[i][j] = GL_NEAREST_MIPMAP_LINEAR;
            state->texture_wrap_s[i][j] = GL_REPEAT;
            state->texture_wrap_t[i][j] = GL_REPEAT;
        }
        state->texture_3d_wrap_r[i] = GL_REPEAT;
    }

    state->buffer_size[0] = state->buffer_size[1] = 0;
    state->buffer_usage[0] = state->buffer_usage[1] = GL_STATIC_DRAW;
    state->texture_cache = NewHashTable(free);

    state->framebuffer_complete = true;
}

egl_state_t *
egl_state_new (EGLDisplay display, EGLContext context)
{
    egl_state_t *new_state = malloc (sizeof (egl_state_t));
    egl_state_init (new_state, display, context);
    return new_state;
}

void
delete_texture_from_name_handler (GLuint key,
                                  void *data,
                                  void *userData)
{
    name_handler_delete_names (1, data);
}

void
egl_state_destroy (void *abstract_state)
{
    egl_state_t *state = abstract_state;

    if (state->vertex_attribs.attribs != state->vertex_attribs.embedded_attribs)
        free (state->vertex_attribs.attribs);

    /* We don't use egl_state_get_texture_cache or egl_state_get_program_list
     * here because we don't want to delete a sharing context's state. */
    HashWalk (state->texture_cache, delete_texture_from_name_handler, NULL);
    DeleteHashTable (state->texture_cache);
    link_list_clear (&state->programs);

    if (state->vendor_string)
        free (state->vendor_string);
    if (state->renderer_string)
        free (state->renderer_string);
    if (state->version_string)
        free (state->version_string);
    if (state->shading_language_version_string)
        free (state->version_string);
    if (state->extensions_string)
        free (state->extensions_string);

    free (state);
}

link_list_t **
cached_gl_states ()
{
    static link_list_t *states = NULL;
    return &states;
}

static HashTable *
egl_state_get_texture_cache (egl_state_t *egl_state)
{
    if (egl_state->share_context)
        return egl_state->share_context->texture_cache;
    return egl_state->texture_cache;
}

texture_t *
egl_state_lookup_cached_texture (egl_state_t *egl_state,
                                 GLuint texture_id)
{
    return (texture_t *) HashLookup (egl_state_get_texture_cache (egl_state), texture_id);
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
void
egl_state_create_cached_texture (egl_state_t *egl_state,
                                 GLuint texture_id)
{
    HashInsert (egl_state_get_texture_cache (egl_state),
                texture_id, _create_texture (texture_id));

}

static link_list_t **
egl_state_get_program_list (egl_state_t *egl_state)
{
    if (egl_state->share_context)
        return &egl_state->share_context->programs;
    return &egl_state->programs;
}

void
egl_state_create_cached_program (egl_state_t *egl_state,
                                 GLuint program_id)
{
    link_list_t **program_list = egl_state_get_program_list (egl_state);
    link_list_append (program_list, program_new (program_id), program_destroy);
}

program_t *
egl_state_lookup_cached_program (egl_state_t *egl_state,
                                 GLuint program_id)
{
    link_list_t **program_list = egl_state_get_program_list (egl_state);
    link_list_t *current = *program_list;
    while (current) {
        program_t *program = (program_t *)current->data;
        if (program->id == program_id)
            return program;
        current = current->next;
    }
    return NULL;
}

void
egl_state_destroy_cached_program (egl_state_t *egl_state,
                                  program_t *program)
{
    link_list_delete_first_entry_matching_data (
        egl_state_get_program_list (egl_state),
        program);
}
