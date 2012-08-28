#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <string.h>
#include <stdlib.h>

#include "gpuprocess_types_private.h"
#include "gpuprocess_thread_private.h"
#include "gpuprocess_egl_states.h"

extern __thread v_link_list_t *active_state;
extern __thread gpu_thread *srv_thread;
extern __thread int unpack_alignment;

static inline v_bool_t
_is_error_state (void )
{
    egl_state_t *state;

    if (! active_state || ! srv_thread)
        return TRUE;

    state = (egl_state_t *) active_state->data;

    if (! state ||
        ! (state->display == EGL_NO_DISPLAY ||
           state->context == EGL_NO_CONTEXT) ||
	state->active == FALSE) {
        return TRUE;
    }

    return FALSE;
}

/* GLES2 core profile API */
void glActiveTexture (GLenum texture)
{
    if (_is_error_state ())
        return;

    /* XXX: post command buffer and no wait */
}

void glAttachShader (GLuint program, GLuint shader)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glBindAttribLocation (GLuint program, GLuint index, const GLchar *name)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glBindBuffer (GLenum target, GLuint buffer)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glBindFramebuffer (GLenum target, GLuint framebuffer)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glBindRenderbuffer (GLenum target, GLuint renderbuffer)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glBindTexture (GLenum target, GLuint texture)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glBlendColor (GLclampf red, GLclampf green,
                   GLclampf blue, GLclampf alpha)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glBlendEquation (GLenum mode)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glBlendFunc (GLenum sfactor, GLenum dfactor)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glBlendFuncSeparate (GLenum srcRGB, GLenum dstRGB,
                          GLenum srcAlpha, GLenum dstAlpha)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glBufferData (GLenum target, GLsizeiptr size,
                   const GLvoid *data, GLenum usage)
{
    GLvoid *data_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in data */
    if (data && size > 0) {
        data_copy = (GLvoid *) malloc (sizeof (char) * size);
        memcpy (data_copy, data, sizeof (char) * size);
    }
}

void glBufferSubData (GLenum target, GLintptr offset,
                      GLsizeiptr size, const GLvoid *data)
{
    GLvoid *data_copy = NULL;
    
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in data */
    if (data && size > 0) {
        data_copy = (GLvoid *) malloc (sizeof (char) * size);
        memcpy (data_copy, data, sizeof (char) * size);
    }
}

GLenum glCheckFramebufferStatus (GLenum target)
{
    GLenum result = GL_INVALID_ENUM;
    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

void glClear (GLbitfield mask)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glClearColor (GLclampf red, GLclampf green,
                   GLclampf blue, GLclampf alpha)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glClearDepthf (GLclampf depth)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glClearStencil (GLint s)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glColorMask (GLboolean red, GLboolean green,
                  GLboolean blue, GLboolean alpha)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

/* total parameters 8 * sizeof (GLint) */
void glCompressedTexImage2D (GLenum target, GLint level,
                             GLenum internalformat,
                             GLsizei width, GLsizei height,
                             GLint border, GLsizei imageSize,
                             const GLvoid *data)
{
    GLvoid *data_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in data */
    if (data && imageSize > 0) {
        data_copy = (GLvoid *) malloc (sizeof (char) * imageSize);
        memcpy (data_copy, data, sizeof (char) * imageSize);
    }
}

/* total parameters 9 * sizeof (GLint) */
void glCompressedTexSubImage2D (GLenum target, GLint level,
                                GLint xoffset, GLint yoffset,
                                GLsizei width, GLsizei height,
                                GLenum format, GLsizei imageSize,
                                const GLvoid *data)
{
    GLvoid *data_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in data */
    if (data && imageSize > 0) {
        data_copy = (GLvoid *) malloc (sizeof (char) * imageSize);
        memcpy (data_copy, data, sizeof (char) * imageSize);
    }
}

/* total parameters 8 * sizeof (GLint) */
void glCopyTexImage2D (GLenum target, GLint level,
                       GLenum internalformat,
                       GLint x, GLint y,
                       GLsizei width, GLsizei height,
                       GLint border)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

/* total parameters 8 * sizeof (GLint) */
void glCopyTexSubImage2D (GLenum target, GLint level,
                          GLint xoffset, GLint yoffset,
                          GLint x, GLint y,
                          GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

/* This is a sync call */
GLuint glCreateProgram (void)
{
    GLuint result = 0;
    
    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

/* sync call */
GLuint glCreateShader (GLenum shaderType)
{
    GLuint result = 0;
    
    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

void glCullFace (GLenum mode)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glDeleteBuffers (GLsizei n, const GLuint *buffers)
{
    GLuint *buffers_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in buffers */
    if (buffers && n > 0) {
        buffers_copy = (GLuint *) malloc (sizeof (GLuint) * n);
        memcpy ((void *)buffers, (const void *)buffers, sizeof (GLuint) * n);
    }
}

void glDeleteFramebuffers (GLsizei n, const GLuint *framebuffers)
{
    GLuint *framebuffers_copy = NULL;
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in framebuffers */
    if (framebuffers && n > 0) {
        framebuffers_copy = (GLuint *) malloc (sizeof (GLuint) * n);
        memcpy ((void *)framebuffers_copy, (const void *)framebuffers, 
                sizeof (GLuint) * n);
    }
}

void glDeleteProgram (GLuint program)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glDeleteRenderbuffers (GLsizei n, const GLuint *renderbuffers)
{
    GLuint *renderbuffers_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in renderbuffers */
    if (renderbuffers && n > 0) {
        renderbuffers_copy = (GLuint *) malloc (sizeof (GLuint) * n);
        memcpy ((void *)renderbuffers_copy, (const void *)renderbuffers, 
                sizeof (GLuint) * n);
    }
}

void glDeleteShader (GLuint shader)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glDeleteTextures (GLsizei n, const GLuint *textures)
{
    GLuint *textures_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in textures */
    if (textures && n > 0) {
        textures_copy = (GLuint *) malloc (sizeof (GLuint) * n);
        memcpy ((void *)textures_copy, (const void *)textures, 
                sizeof (GLuint) * n);
    }
}

void glDepthFunc (GLenum func)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glDepthMask (GLboolean flag)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glDepthRangef (GLclampf nearVal, GLclampf farVal)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glDetachShader (GLuint program, GLuint shader)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glDisable (GLenum cap)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glEnable (GLenum cap)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glDisableVertexAttribArray (GLuint index)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glEnableVertexAttribArray (GLuint index)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glDrawElements (GLenum mode, GLsizei count, GLenum type,
                     const GLvoid *indices)
{
    GLvoid *indices_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy indices.  According to spec, if type is neither 
     * GL_UNSIGNED_BYTE nor GL_UNSIGNED_SHORT, GL_INVALID_ENUM error
     * can be generated, we only support these two types, other types
     * will generate error even the underlying driver supports other
     * types.
     */
    if (indices && count > 0 ) {
        if (type == GL_UNSIGNED_BYTE) {
            indices_copy = (GLvoid *) malloc (sizeof (char) * count);
            memcpy ((void *)indices_copy, (const void *)indices, 
                    sizeof (char) * count);
        }
        else if (type == GL_UNSIGNED_SHORT) {
            indices_copy = (GLvoid *) malloc (sizeof (short) * count);
            memcpy ((void *)indices_copy, (const void *)indices, 
                    sizeof (short) * count);
        }
    }
}

void glFinish (void)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

void glFlush (void)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: flush command buffer, post command and no wait */
}

/* total parameters 4 * sizeof (GLint) */
void glFramebufferRenderbuffer (GLenum target, GLenum attachment,
                                GLenum renderbuffertarget,
                                GLenum renderbuffer)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

/* total parameters 5 * sizeof (GLint) */
void glFramebufferTexture2D (GLenum target, GLenum attachment,
                             GLenum textarget, GLuint texture, GLint level)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glFrontFace (GLenum mode)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glGenBuffers (GLsizei n, GLuint *buffers)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

void glGenFramebuffers (GLsizei n, GLuint *framebuffers)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

void glGenRenderbuffers (GLsizei n, GLuint *renderbuffers)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

void glGenTextures (GLsizei n, GLuint *textures)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

void glGenerateMipmap (GLenum target)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glGetBooleanv (GLenum pname, GLboolean *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

void glGetFloatv (GLenum pname, GLfloat *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

void glGetIntegerv (GLenum pname, GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

/* total parameters 7 * sizeof (GLint) */
void glGetActiveAttrib (GLuint program, GLuint index,
                        GLsizei bufsize, GLsizei *length,
                        GLint *size, GLenum *type, GLchar *name)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

/* total parameters 7 * sizeof (GLint) */
void glGetActiveUniform (GLuint program, GLuint index, GLsizei bufsize,
                         GLsizei *length, GLint *size, GLenum *type,
                         GLchar *name)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

void glGetAttachedShaders (GLuint program, GLsizei maxCount,
                           GLsizei *count, GLuint *shaders)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

GLint glGetAttribLocation (GLuint program, const GLchar *name)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

void glGetBufferParameteriv (GLenum target, GLenum value, GLint *data)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

GLenum glGetError (void)
{
    GLenum error = GL_INVALID_OPERATION;
    
    if (_is_error_state ())
        return error;

    /* XXX: post command and wait */
    return error;
}

void glGetFramebufferAttachmentParameteriv (GLenum target,
                                            GLenum attachment,
                                            GLenum pname,
                                            GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetProgramInfoLog (GLuint program, GLsizei maxLength,
                          GLsizei *length, GLchar *infoLog)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetProgramiv (GLuint program, GLenum pname, GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetRenderbufferParameteriv (GLenum target,
                                   GLenum pname,
                                   GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetShaderInfoLog (GLuint program, GLsizei maxLength,
                         GLsizei *length, GLchar *infoLog)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetShaderPrecisionFormat (GLenum shaderType, GLenum precisionType,
                                 GLint *range, GLint *precision)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetShaderSource (GLuint shader, GLsizei bufSize, GLsizei *length,
                        GLchar *source)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetShaderiv (GLuint shader, GLenum pname, GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

const GLubyte *glGetString (GLenum name)
{
    GLubyte *result = NULL;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

void glGetTexParameteriv (GLenum target, GLenum pname, GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetUniformiv (GLuint program, GLint location, GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetUniformfv (GLuint program, GLint location, GLfloat *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GLint glGetUniformLocation (GLuint program, const GLchar *name)
{
    GLint result = -1;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

void glGetVertexAttribfv (GLuint index, GLenum pname, GLfloat *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetVertexAttribiv (GLuint index, GLenum pname, GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glGetVertexAttribPointerv (GLuint index, GLenum pname, GLvoid **pointer)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glHint (GLenum target, GLenum mode)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

GLboolean glIsBuffer (GLuint buffer)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GLboolean glIsEnabled (GLenum cap)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GLboolean glIsFramebuffer (GLuint framebuffer)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GLboolean glIsProgram (GLuint program)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GLboolean glIsRenderbuffer (GLuint renderbuffer)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GLboolean glIsShader (GLuint shader)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GLboolean glIsTexture (GLuint texture)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

void glLineWidth (GLfloat width)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glLinkProgram (GLuint program)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glPixelStorei (GLenum pname, GLint param)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */

    if (pname == GL_UNPACK_ALIGNMENT && 
        (param == 1 || param == 2 || param == 4 || param == 8))
        unpack_alignment = param;
    return;
}

void glPolygonOffset (GLfloat factor, GLfloat units)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

/* total parameter 7 * sizeof (GLint) */
void glReadPixels (GLint x, GLint y,
                   GLsizei width, GLsizei height,
                   GLenum format, GLenum type, GLvoid *data)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

void glCompileShader (GLuint shader)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glReleaseShaderCompiler (void)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

/* total parameters 4 * sizeof (GLint) */
void glRenderbufferStorage (GLenum target, GLenum internalformat,
                            GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glSampleCoverage (GLclampf value, GLboolean invert)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

/* total parameters 4 * sizeof (GLint) */
void glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}


void glShaderBinary (GLsizei n, const GLuint *shaders,
                     GLenum binaryformat, const void *binary,
                     GLsizei length)
{
    void *binary_copy = NULL;
    GLuint *shaders_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: we must copy data in shaders, binary */
    if (n > 0 && shaders) {
        shaders_copy = (GLuint *)malloc (sizeof (GLuint) * n);
        memcpy ((void *)shaders_copy, (const void *)shaders, 
                sizeof (GLuint) * n);
    }
    if (length > 0 && binary) {
        binary_copy = (void *) malloc (sizeof (char) * length);
        memcpy (binary_copy, binary, sizeof (char) * length);
    }

    return;
}

void glShaderSource (GLuint shader, GLsizei count,
                     const GLchar **string, const GLint *length)
{
    GLchar **string_copy = NULL;
    GLint *length_copy = NULL;
    int i;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in string and length */
    if (count > 0 && string && length) {
        string_copy = (GLchar **) malloc (sizeof (GLchar *) * count );
        length_copy = (GLint *)malloc (sizeof (GLint) * count);
        
        memcpy ((void *)length_copy, (const void *)length,
                sizeof (GLint) * count);

        for (i = 0; i < count; i++) {
            if (length[i] > 0) {
                string_copy[i] = (GLchar *)malloc (sizeof (GLchar) * length[i]);
                memcpy ((void *)string_copy[i], (const void *)string[i],
                        sizeof (GLchar) * length[i]);
            }
        }
    }
     
    return;
}

void glStencilFunc (GLenum func, GLint ref, GLuint mask)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glStencilFuncSeparate (GLenum face, GLenum func,
                            GLint ref, GLuint mask)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glStencilMaskSeparate (GLenum face, GLuint mask)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glStencilMask (GLuint mask)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glStencilOpSeparate (GLenum face, GLenum sfail, GLenum dpfail,
                          GLenum dppass)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glStencilOp (GLenum sfail, GLenum dpfail, GLenum dppass)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

static int 
_gl_get_data_width (GLsizei width, GLenum format, GLenum type)
{
    int padding = 0;
    int mod = 0;
    int total_width = 0;
    
    if (type == GL_UNSIGNED_BYTE) {
        if (format == GL_RGB) {
            mod = width * 3 % unpack_alignment;
            padding = mod == 0 ? 0 : unpack_alignment - mod;
            total_width = width * 3 + padding;
        }
        else if (format == GL_RGBA) {
            mod = width * 4 % unpack_alignment;
            padding = mod == 0 ? 0 : unpack_alignment - mod;
            total_width = width * 4 + padding;
        }
        else if (format = GL_ALPHA || format == GL_LUMINANCE) {
            mod = width  % unpack_alignment;
            padding = mod == 0 ? 0 : unpack_alignment - mod;
            total_width = width + padding;
        }
        else if (format == GL_LUMINANCE_ALPHA) {
            mod = width * 2  % unpack_alignment;
            padding = mod == 0 ? 0 : unpack_alignment - mod;
            total_width = width * 2 + padding;
        }
    }
    else {
        mod = width * 2 % unpack_alignment;
        padding = mod == 0 ? 0 : unpack_alignment - mod;
        total_width = width * 2 + padding;
    }
    
    return total_width;
}

/* total parameters 9 * sizeof (GLint) */
void glTexImage2D (GLenum target, GLint level, GLint internalformat,
                   GLsizei width, GLsizei height, GLint border,
                   GLenum format, GLenum type, const GLvoid *data)
{
    GLvoid *data_copy = NULL;
    int data_width = 0;
    int image_size;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: Is it worth make copy data and make it async call? */
    if (data && width > 0 && height > 0) {
        data_width = _gl_get_data_width (width, format, type);
        image_size = data_width * height;

        if (image_size > 0) {
            data_copy = (GLvoid *)malloc (sizeof (char) * image_size);
            memcpy ((void *)data_copy, (const void *)data, sizeof (char) * image_size);
        }
    } 
            
    return;
}

void glTexParameteri (GLenum target, GLenum pname, GLint param)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glTexParameterf (GLenum target, GLenum pname, GLfloat param)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

/* total parameters 9 * sizeof (GLint) */
void glTexSubImage2D (GLenum target, GLint level,
                      GLint xoffset, GLint yoffset,
                      GLsizei width, GLsizei height,
                      GLenum format, GLenum type, const GLvoid *data)
{
    GLvoid *data_copy = NULL;
    int data_width = 0;
    int image_size;
    
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: Is it worth make copy data and make it async call? */
    if (data && width > 0 && height > 0) {
        data_width = _gl_get_data_width (width, format, type);
        image_size = data_width * height;

        if (image_size > 0) {
            data_copy = (GLvoid *)malloc (sizeof (char) * image_size);
            memcpy ((void *)data_copy, (const void *)data, sizeof (char) * image_size);
        }
    } 
    return;
}

void glUniform1f (GLint location, GLfloat v0)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glUniform2f (GLint location, GLfloat v0, GLfloat v1)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glUniform3f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glUniform4f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2,
                  GLfloat v3)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glUniform1i (GLint location, GLint v0)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glUniform2i (GLint location, GLint v0, GLint v1)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glUniform3i (GLint location, GLint v0, GLint v1, GLint v2)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glUniform4i (GLint location, GLint v0, GLint v1, GLint v2,
                  GLint v3)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}


void glUniform1fv (GLint location, GLsizei count, const GLfloat *value)
{
    GLfloat *value_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in value */
    if (count > 0 && value) {
        value_copy = (GLfloat *) malloc (sizeof (GLfloat) * count);
        memcpy ((void *)value_copy, (const void *)value, sizeof (GLfloat) * count);
    }
    return;
}

void glUniform2fv (GLint location, GLsizei count, const GLfloat *value)
{
    GLfloat *value_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in value */
    if (count > 0 && value) {
        value_copy = (GLfloat *) malloc (sizeof (GLfloat) * count);
        memcpy ((void *)value_copy, (const void *)value, sizeof (GLfloat) * count);
    }
    return;
}

void glUniform3fv (GLint location, GLsizei count, const GLfloat *value)
{
    GLfloat *value_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in value */
    if (count > 0 && value) {
        value_copy = (GLfloat *) malloc (sizeof (GLfloat) * count);
        memcpy ((void *)value_copy, (const void *)value, sizeof (GLfloat) * count);
    }
    return;
}

void glUniform4fv (GLint location, GLsizei count, const GLfloat *value)
{
    GLfloat *value_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in value */
    if (count > 0 && value) {
        value_copy = (GLfloat *) malloc (sizeof (GLfloat) * count);
        memcpy ((void *)value_copy, (const void *)value, sizeof (GLfloat) * count);
    }
    return;
}

void glUniform1iv (GLint location, GLsizei count, const GLint *value)
{
    GLint *value_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in value */
    if (count > 0 && value) {
        value_copy = (GLint *) malloc (sizeof (GLint) * count);
        memcpy ((void *)value_copy, (const void *)value, sizeof (GLint) * count);
    }
    return;
}

void glUniform2iv (GLint location, GLsizei count, const GLint *value)
{
    GLint *value_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in value */
    if (count > 0 && value) {
        value_copy = (GLint *) malloc (sizeof (GLint) * count);
        memcpy ((void *)value_copy, (const void *)value, sizeof (GLint) * count);
    }
    return;
}

void glUniform3iv (GLint location, GLsizei count, const GLint *value)
{
    GLint *value_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in value */
    if (count > 0 && value) {
        value_copy = (GLint *) malloc (sizeof (GLint) * count);
        memcpy ((void *)value_copy, (const void *)value, sizeof (GLint) * count);
    }
    return;
}

void glUniform4iv (GLint location, GLsizei count, const GLint *value)
{
    GLint *value_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in value */
    if (count > 0 && value) {
        value_copy = (GLint *) malloc (sizeof (GLint) * count);
        memcpy ((void *)value_copy, (const void *)value, sizeof (GLint) * count);
    }
    return;
}

void glUseProgram (GLuint program)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glValidateProgram (GLuint program)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glVertexAttrib1f (GLuint index, GLfloat v0)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glVertexAttrib2f (GLuint index, GLfloat v0, GLfloat v1)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glVertexAttrib3f (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glVertexAttrib4f (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2,
                       GLfloat v3)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

void glVertexAttrib1fv (GLuint index, const GLfloat *v)
{
    GLfloat *v_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in v */
    if (v) {
        v_copy = (GLfloat *) malloc (sizeof (GLfloat));
        v_copy[0] = v[0];
    }
    return;
}

void glVertexAttrib2fv (GLuint index, const GLfloat *v)
{
    GLfloat *v_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in v */
    if (v) {
        v_copy = (GLfloat *) malloc (sizeof (GLfloat) * 2);
        memcpy ((void *)v_copy, (const void *)v, sizeof (GLfloat *) * 2);
    }
    return;
}

void glVertexAttrib3fv (GLuint index, const GLfloat *v)
{
    GLfloat *v_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in v */
    if (v) {
        v_copy = (GLfloat *) malloc (sizeof (GLfloat) * 3);
        memcpy ((void *)v_copy, (const void *)v, sizeof (GLfloat *) * 3);
    }
    return;
}

void glVertexAttrib4fv (GLuint index, const GLfloat *v)
{
    GLfloat *v_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* copy data in v */
    if (v) {
        v_copy = (GLfloat *) malloc (sizeof (GLfloat) * 4);
        memcpy ((void *)v_copy, (const void *)v, sizeof (GLfloat *) * 3);
    }
    return;
}

/* total parameters 6 * sizeof (GLint) */
void glVertexAttribPointer (GLuint index, GLint size, GLenum type,
                            GLboolean normalized, GLsizei stride,
                            const GLvoid *pointer)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: no need to copy data in pointer */
    return;
}

void glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}
/* end of GLES2 core profile */

#ifdef GL_OES_EGL_image
GL_APICALL void GL_APIENTRY
glEGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

GL_APICALL void GL_APIENTRY
glEGLImageTargetRenderbufferStorageOES (GLenum target, GLeglImageOES image)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}
#endif

#ifdef GL_OES_get_program_binary
/* total parameters size 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glGetProgramBinaryOES (GLuint program, GLsizei bufSize, GLsizei *length,
                       GLenum *binaryFormat, GLvoid *binary)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glProgramBinaryOES (GLuint program, GLenum binaryFormat,
                    const GLvoid *binary, GLint length)
{
    GLvoid *binary_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in binary */
    if (length > 0 && binary) {
        binary_copy = (void *) malloc (sizeof (char) * length);
        memcpy (binary_copy, binary, sizeof (char) * length);
    }
    return;
}
#endif

#ifdef GL_OES_mapbuffer
GL_APICALL void* GL_APIENTRY glMapBufferOES (GLenum target, GLenum access)
{
    void *result = NULL;
    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GL_APICALL GLboolean GL_APIENTRY
glUnmapBufferOES (GLenum target)
{
    GLboolean result = GL_FALSE;
    
    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GL_APICALL void GL_APIENTRY
glGetBufferPointervOES (GLenum target, GLenum pname, GLvoid **params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}
#endif

#ifdef GL_OES_texture_3D
/* total parameters 10 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glTexImage3DOES (GLenum target, GLint level, GLenum internalformat,
                 GLsizei width, GLsizei height, GLsizei depth,
                 GLint border, GLenum format, GLenum type,
                 const GLvoid *pixels)
{
    GLvoid *pixels_copy = NULL;
    int pixels_width = 0;
    int image_size;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: Is it worth make copy data and make it async call?,
     * I am not clear whether we need padding for depth. 
     * no mentioning in the spec: 
     * http://www.khronos.org/registry/gles/extensions/OES/OES_texture_3D.txt
     */
    if (pixels && width > 0 && height > 0 && depth > 0 ) {
        pixels_width = _gl_get_data_width (width, format, type);
        image_size = pixels_width * height * depth;

        if (image_size > 0) {
            pixels_copy = (GLvoid *)malloc (sizeof (char) * image_size);
            memcpy ((void *)pixels_copy, (const void *)pixels, sizeof (char) * image_size);
        }
    } 
            
    return;
}

/* total parameters 11 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glTexSubImage3DOES (GLenum target, GLint level,
                    GLint xoffset, GLint yoffset, GLint zoffset,
                    GLsizei width, GLsizei height, GLsizei depth,
                    GLenum format, GLenum type, const GLvoid *data)
{
    GLvoid *data_copy = NULL;
    int data_width = 0;
    int image_size;
    
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: Is it worth make copy data and make it async call?
     * I am not clear whether we need padding for depth. 
     * no mentioning in the spec: 
     * http://www.khronos.org/registry/gles/extensions/OES/OES_texture_3D.txt
     */
    if (data && width > 0 && height > 0 && depth) {
        data_width = _gl_get_data_width (width, format, type);
        image_size = data_width * height * depth;

        if (image_size > 0) {
            data_copy = (GLvoid *)malloc (sizeof (char) * image_size);
            memcpy ((void *)data_copy, (const void *)data, sizeof (char) * image_size);
        }
    } 
    return;
}

/* total parameters 9 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glCopyTexSubImage3DOES (GLenum target, GLint level,
                        GLint xoffset, GLint yoffset, GLint zoffset,
                        GLint x, GLint y,
                        GLint width, GLint height)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

GL_APICALL void GL_APIENTRY
glCompressedTexImage3DOES (GLenum target, GLint level,
                           GLenum internalformat,
                           GLsizei width, GLsizei height, GLsizei depth,
                           GLint border, GLsizei imageSize,
                           const GLvoid *data)
{
    GLvoid *data_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in data */
    if (data && imageSize > 0) {
        data_copy = (GLvoid *) malloc (sizeof (char) * imageSize);
        memcpy (data_copy, data, sizeof (char) * imageSize);
    }
}

/* total parameters 11 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glCompressedTexSubImage3DOES (GLenum target, GLint level,
                              GLint xoffset, GLint yoffset, GLint zoffset,
                              GLsizei width, GLsizei height, GLsizei depth,
                              GLenum format, GLsizei imageSize,
                              const GLvoid *data)
{
    GLvoid *data_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in data */
    if (data && imageSize > 0) {
        data_copy = (GLvoid *) malloc (sizeof (char) * imageSize);
        memcpy (data_copy, data, sizeof (char) * imageSize);
    }
        
    return;
}

/* total parameters 6 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glFramebufferTexture3DOES (GLenum target, GLenum attachment,
                           GLenum textarget, GLuint texture,
                           GLint level, GLint zoffset)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}
#endif

#ifdef GL_OES_vertex_array_object
GL_APICALL void GL_APIENTRY
glBindVertexArrayOES (GLuint array)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

GL_APICALL void GL_APIENTRY
glDeleteVertexArraysOES (GLsizei n, const GLuint *arrays)
{
    GLuint *arrays_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in arrays */
    if (n > 0 && arrays) {
        arrays_copy = (GLuint *) malloc (sizeof (GLuint) * n);
        memcpy ((void *)arrays_copy, (const void *)arrays,
                sizeof (GLuint) * n);
    }
    return;
}

GL_APICALL void GL_APIENTRY
glGenVertexArraysOES (GLsizei n, GLuint *arrays)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL GLboolean GL_APIENTRY
glIsVertexArrayOES (GLuint array)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}
#endif

#ifdef GL_AMD_performance_monitor
GL_APICALL void GL_APIENTRY
glGetPerfMonitorGroupsAMD (GLint *numGroups, GLsizei groupSize, 
                           GLuint *groups)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

/* total parameters 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glGetPerfMonitorCountersAMD (GLuint group, GLint *numCounters, 
                             GLint *maxActiveCounters, GLsizei counterSize,
                             GLuint *counters)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glGetPerfMonitorGroupStringAMD (GLuint group, GLsizei bufSize, 
                                GLsizei *length, GLchar *groupString)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

/* total parameters 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glGetPerfMonitorCounterStringAMD (GLuint group, GLuint counter, 
                                  GLsizei bufSize, 
                                  GLsizei *length, GLchar *counterString)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glGetPerfMonitorCounterInfoAMD (GLuint group, GLuint counter, 
                                GLenum pname, GLvoid *data)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glGenPerfMonitorsAMD (GLsizei n, GLuint *monitors) 
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glDeletePerfMonitorsAMD (GLsizei n, GLuint *monitors) 
{
    GLuint *monitors_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in monitors */
    if (n > 0 && monitors) {
        monitors_copy = (GLuint *) malloc (sizeof (GLuint) * n);
        memcpy ((void *)monitors_copy, (const void *)monitors,
                sizeof (GLuint) * n);
    }
    return;
}

/* total parameter 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glSelectPerfMonitorCountersAMD (GLuint monitor, GLboolean enable,
                                GLuint group, GLint numCounters,
                                GLuint *countersList) 
{
    GLuint *counters_list_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in counterisList */
    if (numCounters > 0 && countersList) {
        counters_list_copy = (GLuint *) malloc (sizeof (GLuint) * numCounters);
        memcpy ((void *)counters_list_copy, (const void *)countersList,
                sizeof (GLuint) * numCounters);
    }
    return;
}

GL_APICALL void GL_APIENTRY
glBeginPerfMonitorAMD (GLuint monitor)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

GL_APICALL void GL_APIENTRY
glEndPerfMonitorAMD (GLuint monitor)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

/* total parameters 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glGetPerfMonitorCounterDataAMD (GLuint monitor, GLenum pname,
                                GLsizei dataSize, GLuint *data,
                                GLint *bytesWritten)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}
#endif

#ifdef GL_ANGLE_framebuffer_blit
/* total parameters 10 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glBlitFramebufferANGLE (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                        GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                        GLbitfield mask, GLenum filter)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}
#endif

#ifdef GL_ANGLE_framebuffer_multisample
/* total parameters 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glRenderbufferStorageMultisampleANGLE (GLenum target, GLsizei samples, 
                                       GLenum internalformat, 
                                       GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}
#endif

#ifdef GL_APPLE_framebuffer_multisample
/* total parameters 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glRenderbufferStorageMultisampleAPPLE (GLenum target, GLsizei samples, 
                                       GLenum internalformat, 
                                       GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

GL_APICALL void GL_APIENTRY
glResolveMultisampleFramebufferAPPLE (void) 
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}
#endif

#ifdef GL_EXT_discard_framebuffer
GL_APICALL void GL_APIENTRY
glDiscardFramebufferEXT (GLenum target, GLsizei numAttachments,
                         const GLenum *attachments)
{
    GLenum *attachments_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in attachments */
    if (numAttachments > 0 && attachments) {
        attachments_copy = (GLenum *)malloc (sizeof (GLenum) * numAttachments);
        memcpy ((void *)attachments_copy, (const void *)attachments,
                sizeof (GLenum) * numAttachments);
    }
}
#endif

#ifdef GL_EXT_multi_draw_arrays
GL_APICALL void GL_APIENTRY
glMultiDrawArraysEXT (GLenum mode, const GLint *first, 
                      const GLsizei *count, GLsizei primcount)
{
    /* not implemented */
}

GL_APICALL void GL_APIENTRY
glMultiDrawElementsEXT (GLenum mode, const GLsizei *count, GLenum type,
                        const GLvoid **indices, GLsizei primcount)
{
    /* not implemented */
}
#endif

#ifdef GL_EXT_multisampled_render_to_texture
/* total parameters 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glRenderbufferStorageMultisampleEXT (GLenum target, GLsizei samples,
                                     GLenum internalformat,
                                     GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

/* total parameters 6 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glFramebufferTexture2DMultisampleEXT (GLenum target, GLenum attachment,
                                      GLenum textarget, GLuint texture,
                                      GLint level, GLsizei samples)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}
#endif

#ifdef GL_IMG_multisampled_render_to_texture
/* total parameters 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glRenderbufferStorageMultisampleIMG (GLenum target, GLsizei samples,
                                     GLenum internalformat,
                                     GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

/* total parameter 6 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glFramebufferTexture2DMultisampleIMG (GLenum target, GLenum attachment,
                                      GLenum textarget, GLuint texture,
                                      GLint level, GLsizei samples)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}
#endif

#ifdef GL_NV_fence
GL_APICALL void GL_APIENTRY
glDeleteFencesNV (GLsizei n, const GLuint *fences)
{
    GLuint *fences_copy = NULL;

    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    /* XXX: copy data in fences */
    if (fences && n > 0) {
        fences_copy = (GLuint *)malloc (sizeof (GLuint) * n);
        memcpy ((void *)fences_copy, (const void *)fences, 
                sizeof (GLuint) * n);
    }
}

GL_APICALL void GL_APIENTRY
glGenFencesNV (GLsizei n, GLuint *fences)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
}

GL_APICALL GLboolean GL_APIENTRY
glIsFenceNV (GLuint fence)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GL_APICALL GLboolean GL_APIENTRY
glTestFenceNV (GLuint fence)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GL_APICALL GLboolean GL_APIENTRY
glGetFenceivNV (GLuint fence, GLenum pname, int *params)
{
    GLboolean result = GL_FALSE;

    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GL_APICALL void GL_APIENTRY
glFinishFenceivNV (GLuint fence)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

GL_APICALL void GL_APIENTRY
glSetFenceNV (GLuint fence, GLenum condition)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}
#endif

#ifdef GL_NV_coverage_sample
GL_APICALL void GL_APIENTRY
glCoverageMaskNV (GLboolean mask)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

GL_APICALL void GL_APIENTRY
glCoverageOperationNV (GLenum operation)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}
#endif

#ifdef GL_QCOM_driver_control
GL_APICALL void GL_APIENTRY
glGetDriverControlsQCOM (GLint *num, GLsizei size, GLuint *driverControls)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glGetDriverControlStringQCOM (GLuint driverControl, GLsizei bufSize,
                              GLsizei *length, GLchar *driverControlString)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glEnableDriverControlQCOM (GLuint driverControl)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

GL_APICALL void GL_APIENTRY
glDisableDriverControlQCOM (GLuint driverControl)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}
#endif

#ifdef GL_QCOM_extended_get
GL_APICALL void GL_APIENTRY
glExtGetTexturesQCOM (GLuint *textures, GLint maxTextures, 
                      GLint *numTextures)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glExtGetBuffersQCOM (GLuint *buffers, GLint maxBuffers, GLint *numBuffers) 
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glExtGetRenderbuffersQCOM (GLuint *renderbuffers, GLint maxRenderbuffers,
                           GLint *numRenderbuffers)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glExtGetFramebuffersQCOM (GLuint *framebuffers, GLint maxFramebuffers,
                          GLint *numFramebuffers)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glExtGetTexLevelParameterivQCOM (GLuint texture, GLenum face, GLint level,
                                 GLenum pname, GLint *params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glExtTexObjectStateOverrideiQCOM (GLenum target, GLenum pname, GLint param)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

/* total parameter 11 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glExtGetTexSubImageQCOM (GLenum target, GLint level,
                         GLint xoffset, GLint yoffset, GLint zoffset,
                         GLsizei width, GLsizei height, GLsizei depth,
                         GLenum format, GLenum type, GLvoid *texels)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glExtGetBufferPointervQCOM (GLenum target, GLvoid **params)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}
#endif

#ifdef GL_QCOM_extended_get2
GL_APICALL void GL_APIENTRY
glExtGetShadersQCOM (GLuint *shaders, GLint maxShaders, GLint *numShaders)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL void GL_APIENTRY
glExtGetProgramsQCOM (GLuint *programs, GLint maxPrograms,
                      GLint *numPrograms)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}

GL_APICALL GLboolean GL_APIENTRY
glExtIsProgramBinaryQCOM (GLuint program)
{
    GLboolean result = FALSE;
    
    if (_is_error_state ())
        return result;

    /* XXX: post command and wait */
    return result;
}

GL_APICALL void GL_APIENTRY
glExtGetProgramBinarySourceQCOM (GLuint program, GLenum shadertype,
                                 GLchar *source, GLint *length)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and wait */
    return;
}
#endif

#ifdef GL_QCOM_tiled_rendering
/* total parameter 5 * sizeof (GLint) */
GL_APICALL void GL_APIENTRY
glStartTilingQCOM (GLuint x, GLuint y, GLuint width, GLuint height,
                   GLbitfield preserveMask)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}

GL_APICALL void GL_APIENTRY
glEndTilingQCOM (GLbitfield preserveMask)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}
#endif
