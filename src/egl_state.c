#include "config.h"
#include "egl_state.h"
#include <stdlib.h>
#include <string.h>

void
egl_state_init (egl_state_t *state)
{
    state->context = EGL_NO_CONTEXT;
    state->display = EGL_NO_DISPLAY;
    state->drawable = EGL_NO_SURFACE;
    state->readable = EGL_NO_SURFACE;

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
    state->max_renderbuffer_size = 1;
    state->max_texture_image_units = 8;
    state->max_texture_size = 64;
    state->max_varying_vectors = 8;
    state->max_vertex_uniform_vectors = 128;
    state->max_vertex_texture_image_units = 0;

    state->error = GL_NO_ERROR;
    state->need_get_error = false;
    state->programs = NULL;
    /* We add a head to the list so we can get a reference */
    program_t *head = (program_t *)malloc (sizeof (program_t));
    head->id = 0;
    head->mark_for_deletion = false;
    head->uniform_location_cache = NULL;
    head->attrib_location_cache = NULL;
    link_list_append (&state->programs, head);


    state->active_texture = GL_TEXTURE0;
    state->array_buffer_binding = 0;

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
            state->texture_wrap_s[i][j] = GL_REPEAT;
            state->texture_wrap_t[i][j] = GL_REPEAT;
        }
        state->texture_3d_wrap_r[i] = GL_REPEAT;
    }

    state->buffer_size[0] = state->buffer_size[1] = 0;
    state->buffer_usage[0] = state->buffer_usage[1] = GL_STATIC_DRAW;
    state->texture_cache = NewHashTable(free);
}

egl_state_t *
egl_state_new ()
{
    egl_state_t *new_state = malloc (sizeof (egl_state_t));
    egl_state_init (new_state);
    return new_state;
}

void
egl_state_destroy (egl_state_t *state)
{
    if (state->vertex_attribs.attribs != state->vertex_attribs.embedded_attribs)
        free (state->vertex_attribs.attribs);

    link_list_t *program_list = state->programs;
    while (program_list) {
        program_t *program = (program_t *)program_list->data;
        DeleteHashTable (program->uniform_location_cache);
        program->uniform_location_cache = NULL;

        DeleteHashTable (program->attrib_location_cache);
        program->attrib_location_cache = NULL;

        free (program);
        link_list_t *temp = program_list;
        program_list = program_list->next;
        free (temp);
    }

    DeleteHashTable (state->texture_cache);
    state->texture_cache = NULL;
    state->programs = NULL;
}

link_list_t **
cached_gl_states ()
{
    static link_list_t *states = NULL;
    return &states;
}

HashTable *
egl_state_get_texture_cache (egl_state_t *egl_state)
{
    if (egl_state->share_context)
        return egl_state->share_context->texture_cache;
    return egl_state->texture_cache;
}
