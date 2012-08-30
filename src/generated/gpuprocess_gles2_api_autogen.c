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

void glCompileShader (GLuint shader)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
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

void glCullFace (GLenum mode)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glDeleteProgram (GLuint program)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glDeleteShader (GLuint shader)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
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

void glDisableVertexAttribArray (GLuint index)
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

void glEnable (GLenum cap)
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

void glGenerateMipmap (GLenum target)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
}

void glHint (GLenum target, GLenum mode)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
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

void glPolygonOffset (GLfloat factor, GLfloat units)
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

void glStencilMask (GLuint mask)
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

void glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
    if (_is_error_state ())
        return;

    /* XXX: post command and no wait */
    return;
}
