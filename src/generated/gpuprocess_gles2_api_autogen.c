#include "gpuprocess_gles2_api_private.h"
void glActiveTexture (GLenum texture)
{
    if (_is_error_state ())
        return;
}

void glAttachShader (GLuint program, GLuint shader)
{
    if (_is_error_state ())
        return;
}

void glBindAttribLocation (GLuint program, GLuint index, const char* name)
{
    if (_is_error_state ())
        return;
}

void glBindBuffer (GLenum target, GLuint buffer)
{
    if (_is_error_state ())
        return;
}

void glBindFramebuffer (GLenum target, GLuint framebuffer)
{
    if (_is_error_state ())
        return;
}

void glBindRenderbuffer (GLenum target, GLuint renderbuffer)
{
    if (_is_error_state ())
        return;
}

void glBindTexture (GLenum target, GLuint texture)
{
    if (_is_error_state ())
        return;
}

void glBlendColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    if (_is_error_state ())
        return;
}

void glBlendEquation (GLenum mode)
{
    if (_is_error_state ())
        return;
}

void glBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha)
{
    if (_is_error_state ())
        return;
}

void glBlendFunc (GLenum sfactor, GLenum dfactor)
{
    if (_is_error_state ())
        return;
}

void glBlendFuncSeparate (
    GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
    if (_is_error_state ())
        return;
}

void glClear (GLbitfield mask)
{
    if (_is_error_state ())
        return;
}

void glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    if (_is_error_state ())
        return;
}

void glClearDepthf (GLclampf depth)
{
    if (_is_error_state ())
        return;
}

void glClearStencil (GLint s)
{
    if (_is_error_state ())
        return;
}

void glColorMask (
    GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    if (_is_error_state ())
        return;
}

void glCompileShader (GLuint shader)
{
    if (_is_error_state ())
        return;
}

void glCopyTexImage2D (
    GLenum target, GLint level, GLenum internalformat, GLint x, GLint y,
    GLsizei width, GLsizei height, GLint border)
{
    if (_is_error_state ())
        return;
}

void glCopyTexSubImage2D (
    GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y,
    GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;
}

void glCullFace (GLenum mode)
{
    if (_is_error_state ())
        return;
}

void glDeleteProgram (GLuint program)
{
    if (_is_error_state ())
        return;
}

void glDeleteShader (GLuint shader)
{
    if (_is_error_state ())
        return;
}

void glDepthFunc (GLenum func)
{
    if (_is_error_state ())
        return;
}

void glDepthMask (GLboolean flag)
{
    if (_is_error_state ())
        return;
}

void glDepthRangef (GLclampf zNear, GLclampf zFar)
{
    if (_is_error_state ())
        return;
}

void glDetachShader (GLuint program, GLuint shader)
{
    if (_is_error_state ())
        return;
}

void glDisable (GLenum cap)
{
    if (_is_error_state ())
        return;
}

void glDisableVertexAttribArray (GLuint index)
{
    if (_is_error_state ())
        return;
}

void glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
    if (_is_error_state ())
        return;
}

void glEnable (GLenum cap)
{
    if (_is_error_state ())
        return;
}

void glEnableVertexAttribArray (GLuint index)
{
    if (_is_error_state ())
        return;
}

void glFinish (void)
{
    if (_is_error_state ())
        return;
}

void glFlush (void)
{
    if (_is_error_state ())
        return;
}

void glFramebufferRenderbuffer (
    GLenum target, GLenum attachment, GLenum renderbuffertarget,
    GLuint renderbuffer)
{
    if (_is_error_state ())
        return;
}

void glFramebufferTexture2D (
    GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
    GLint level)
{
    if (_is_error_state ())
        return;
}

void glFrontFace (GLenum mode)
{
    if (_is_error_state ())
        return;
}

void glGenerateMipmap (GLenum target)
{
    if (_is_error_state ())
        return;
}

void glHint (GLenum target, GLenum mode)
{
    if (_is_error_state ())
        return;
}

void glLineWidth (GLfloat width)
{
    if (_is_error_state ())
        return;
}

void glLinkProgram (GLuint program)
{
    if (_is_error_state ())
        return;
}

void glPolygonOffset (GLfloat factor, GLfloat units)
{
    if (_is_error_state ())
        return;
}

void glReleaseShaderCompiler (void)
{
    if (_is_error_state ())
        return;
}

void glRenderbufferStorage (
    GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;
}

void glSampleCoverage (GLclampf value, GLboolean invert)
{
    if (_is_error_state ())
        return;
}

void glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;
}

void glStencilFunc (GLenum func, GLint ref, GLuint mask)
{
    if (_is_error_state ())
        return;
}

void glStencilFuncSeparate (GLenum face, GLenum func, GLint ref, GLuint mask)
{
    if (_is_error_state ())
        return;
}

void glStencilMask (GLuint mask)
{
    if (_is_error_state ())
        return;
}

void glStencilMaskSeparate (GLenum face, GLuint mask)
{
    if (_is_error_state ())
        return;
}

void glStencilOp (GLenum fail, GLenum zfail, GLenum zpass)
{
    if (_is_error_state ())
        return;
}

void glStencilOpSeparate (GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
    if (_is_error_state ())
        return;
}

void glTexParameteri (GLenum target, GLenum pname, GLint param)
{
    if (_is_error_state ())
        return;
}

void glTexParameterf (GLenum target, GLenum pname, GLfloat param)
{
    if (_is_error_state ())
        return;
}

void glUniform1f (GLint location, GLfloat x)
{
    if (_is_error_state ())
        return;
}

void glUniform1i (GLint location, GLint x)
{
    if (_is_error_state ())
        return;
}

void glUniform2f (GLint location, GLfloat x, GLfloat y)
{
    if (_is_error_state ())
        return;
}

void glUniform2i (GLint location, GLint x, GLint y)
{
    if (_is_error_state ())
        return;
}

void glUniform3f (GLint location, GLfloat x, GLfloat y, GLfloat z)
{
    if (_is_error_state ())
        return;
}

void glUniform3i (GLint location, GLint x, GLint y, GLint z)
{
    if (_is_error_state ())
        return;
}

void glUniform4f (GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    if (_is_error_state ())
        return;
}

void glUniform4i (GLint location, GLint x, GLint y, GLint z, GLint w)
{
    if (_is_error_state ())
        return;
}

void glUseProgram (GLuint program)
{
    if (_is_error_state ())
        return;
}

void glValidateProgram (GLuint program)
{
    if (_is_error_state ())
        return;
}

void glVertexAttrib1f (GLuint indx, GLfloat x)
{
    if (_is_error_state ())
        return;
}

void glVertexAttrib2f (GLuint indx, GLfloat x, GLfloat y)
{
    if (_is_error_state ())
        return;
}

void glVertexAttrib3f (GLuint indx, GLfloat x, GLfloat y, GLfloat z)
{
    if (_is_error_state ())
        return;
}

void glVertexAttrib4f (GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    if (_is_error_state ())
        return;
}

void glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;
}

