#ifndef GPUPROCESS_PROGRAM_H
#define GPUPROCESS_PROGRAM_H

#include "hash.h"
#include "program.h"
#include "thread_private.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdbool.h>

typedef struct v_program_status {
    GLboolean    delete_status;
    GLboolean    link_status;
    GLboolean    validate_status;
    GLint        info_log_length;                 /* initial 0 */
    GLint        attached_shaders;
    GLint        active_attributes;
    GLint        active_attribute_max_length;     /* longest name + NULL */
    GLint        active_uniforms;
    GLint        active_uniform_max_length;       /* longest name + NULL */
} v_program_status_t;

/* this struct for attribute */
typedef struct v_program_attrib {
    GLuint        location;
    GLuint        index;
    GLchar        *name;
} v_program_attrib_t;

typedef struct v_program_attrib_list {
    int                 count;
    v_program_attrib_t  *attribs;
} v_program_attrib_list_t;

typedef struct v_program_uniform {
    GLuint        location;
    GLchar        *name;
    GLchar        *value;
    GLint         num_bytes;        /* number of bytes occupied by value */
} v_program_uniform_t;

typedef struct v_program_uniform_list {
    int                        count;
    v_program_uniform_t *uniforms;
} v_program_uniform_list_t;

typedef struct v_program {
    GLuint                      program;
    v_program_status_t          status;
    v_program_attrib_list_t     attribs;
    v_program_uniform_list_t    uniforms;
} v_program_t;

typedef struct _program {
    GLuint       id;
    bool         mark_for_deletion;
    HashTable    *attrib_location_cache;
    HashTable    *uniform_location_cache;
} program_t;

private program_t*
program_new (GLuint id);

#endif