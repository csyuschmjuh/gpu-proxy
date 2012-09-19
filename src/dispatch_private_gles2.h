#ifndef GPUPROCESS_DISPATCH_PRIVATE_GLES2_H
#define GPUPROCESS_DISPATCH_PRIVATE_GLES2_H

#ifdef HAS_GLES2

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

typedef struct _dispatch {
    EGLint (*eglGetError) (void);

    EGLDisplay (*eglGetDisplay) (EGLNativeDisplayType);
    EGLBoolean (*eglInitialize) (EGLDisplay, EGLint *, EGLint *);
    EGLBoolean (*eglTerminate) (EGLDisplay);

    const char * (*eglQueryString) (EGLDisplay, EGLint);

    EGLBoolean (*eglGetConfigs) (EGLDisplay, EGLConfig *,
                                 EGLint, EGLint *);
    EGLBoolean (*eglChooseConfig) (EGLDisplay, const EGLint *,
                                   EGLConfig *, EGLint,
                                   EGLint *);
    EGLBoolean (*eglGetConfigAttrib) (EGLDisplay, EGLConfig,
                                      EGLint, EGLint *);

    EGLSurface (*eglCreateWindowSurface) (EGLDisplay, EGLConfig,
                                          EGLNativeWindowType,
                                          const EGLint *);
    EGLSurface (*eglCreatePbufferSurface) (EGLDisplay, EGLConfig,
                                           const EGLint *);
    EGLSurface (*eglCreatePixmapSurface) (EGLDisplay, EGLConfig,
                                          EGLNativePixmapType,
                                          const EGLint *);
    EGLBoolean (*eglDestroySurface) (EGLDisplay, EGLSurface);
    EGLBoolean (*eglQuerySurface) (EGLDisplay, EGLSurface,
                                   EGLint, EGLint *);

    EGLBoolean (*eglBindAPI) (EGLenum);
    EGLenum (*eglQueryAPI) (void);

    EGLBoolean (*eglWaitClient) (void);

    EGLBoolean (*eglReleaseThread) (void);

    EGLSurface (*eglCreatePbufferFromClientBuffer) (
        EGLDisplay, EGLenum, EGLClientBuffer,
        EGLConfig, const EGLint *);

    EGLBoolean (*eglSurfaceAttrib) (EGLDisplay, EGLSurface,
                                    EGLint, EGLint);
    EGLBoolean (*eglBindTexImage) (EGLDisplay, EGLSurface, EGLint);
    EGLBoolean (*eglReleaseTexImage) (EGLDisplay, EGLSurface, EGLint);

    EGLBoolean (*eglSwapInterval) (EGLDisplay, EGLint);

    EGLContext (*eglCreateContext) (EGLDisplay, EGLConfig,
                                    EGLContext,
                                    const EGLint *);
    EGLBoolean (*eglDestroyContext) (EGLDisplay, EGLContext);
    EGLBoolean (*eglMakeCurrent) (EGLDisplay, EGLSurface,
                                  EGLSurface, EGLContext);

    EGLContext (*eglGetCurrentContext) (void);
    EGLSurface (*eglGetCurrentSurface) (EGLint);
    EGLDisplay (*eglGetCurrentDisplay) (void);
    EGLBoolean (*eglQueryContext) (EGLDisplay, EGLContext,
                                   EGLint, EGLint *);

    EGLBoolean (*eglWaitGL) (void);
    EGLBoolean (*eglWaitNative) (EGLint engine);
    EGLBoolean (*eglSwapBuffers) (EGLDisplay, EGLSurface);
    EGLBoolean (*eglCopyBuffers) (EGLDisplay, EGLSurface,
                                  EGLNativePixmapType);

    FunctionPointerType (*eglGetProcAddress) (const char *procname);

    void        (*glActiveTexture)(GLenum);
    void        (*glAttachShader)(GLuint, GLuint);
    void        (*glBindAttribLocation)(GLuint, GLuint, const GLchar*);
    void        (*glBindBuffer)(GLenum, GLuint);
    void        (*glBindFramebuffer)(GLenum, GLuint);
    void        (*glBindRenderbuffer)(GLenum, GLuint);
    void        (*glBindTexture)(GLenum, GLuint);
    void        (*glBlendColor)(GLclampf, GLclampf, GLclampf, GLclampf);
    void        (*glBlendEquation)(GLenum);
    void        (*glBlendEquationSeparate)(GLenum, GLenum);
    void        (*glBlendFunc)(GLenum, GLenum);
    void        (*glBlendFuncSeparate)(GLenum, GLenum, GLenum, GLenum);
    void        (*glBufferData)(GLenum, GLsizeiptr, const GLvoid*, GLenum);
    void        (*glBufferSubData)(GLenum, GLintptr, GLsizeiptr, const GLvoid*);
    GLenum      (*glCheckFramebufferStatus)(GLenum);
    void        (*glClear)(GLbitfield);
    void        (*glClearColor)(GLclampf, GLclampf, GLclampf, GLclampf);
    void        (*glClearDepthf)(GLclampf);
    void        (*glClearStencil)(GLint);
    void        (*glColorMask)(GLboolean, GLboolean, GLboolean, GLboolean);
    void        (*glCompileShader)(GLuint);
    void        (*glCompressedTexImage2D)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
    void        (*glCompressedTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
    void        (*glCopyTexImage2D)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
    void        (*glCopyTexSubImage2D)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
    GLuint      (*glCreateProgram)(void);
    GLuint      (*glCreateShader)(GLenum);
    void        (*glCullFace)(GLenum);
    void        (*glDeleteBuffers)(GLsizei, const GLuint*);
    void        (*glDeleteFramebuffers)(GLsizei, const GLuint*);
    void        (*glDeleteProgram)(GLuint);
    void        (*glDeleteRenderbuffers)(GLsizei, const GLuint*);
    void        (*glDeleteShader)(GLuint);
    void        (*glDeleteTextures)(GLsizei, const GLuint*);
    void        (*glDepthFunc)(GLenum);
    void        (*glDepthMask)(GLboolean);
    void        (*glDepthRangef)(GLclampf, GLclampf);
    void        (*glDetachShader)(GLuint, GLuint);
    void        (*glDisable)(GLenum);
    void        (*glDisableVertexAttribArray)(GLuint index);
    void        (*glDrawArrays)(GLenum mode, GLint first, GLsizei count);
    void        (*glDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
    void        (*glEnable)(GLenum cap);
    void        (*glEnableVertexAttribArray)(GLuint index);
    void        (*glFinish)(void);
    void        (*glFlush)(void);
    void        (*glFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    void        (*glFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    void        (*glFrontFace)(GLenum mode);
    void        (*glGenBuffers)(GLsizei n, GLuint* buffers);
    void        (*glGenerateMipmap)(GLenum target);
    void        (*glGenFramebuffers)(GLsizei n, GLuint* framebuffers);
    void        (*glGenRenderbuffers)(GLsizei n, GLuint* renderbuffers);
    void        (*glGenTextures)(GLsizei n, GLuint* textures);
    void        (*glGetActiveAttrib)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
    void        (*glGetActiveUniform)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
    void        (*glGetAttachedShaders)(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
    int         (*glGetAttribLocation)(GLuint program, const GLchar* name);
    void        (*glGetBooleanv)(GLenum pname, GLboolean* params);
    void        (*glGetBufferParameteriv)(GLenum target, GLenum pname, GLint* params);
    GLenum      (*glGetError)(void);
    void        (*glGetFloatv)(GLenum pname, GLfloat* params);
    void        (*glGetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint* params);
    void        (*glGetIntegerv)(GLenum pname, GLint* params);
    void        (*glGetProgramiv)(GLuint program, GLenum pname, GLint* params);
    void        (*glGetProgramInfoLog)(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog);
    void        (*glGetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint* params);
    void        (*glGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
    void        (*glGetShaderInfoLog)(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog);
    void        (*glGetShaderPrecisionFormat)(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
    void        (*glGetShaderSource)(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source);
    const GLubyte* (*glGetString)(GLenum name);
    void        (*glGetTexParameterfv)(GLenum target, GLenum pname, GLfloat* params);
    void        (*glGetTexParameteriv)(GLenum target, GLenum pname, GLint* params);
    void        (*glGetUniformfv)(GLuint program, GLint location, GLfloat* params);
    void        (*glGetUniformiv)(GLuint program, GLint location, GLint* params);
    int         (*glGetUniformLocation)(GLuint program, const GLchar* name);
    void        (*glGetVertexAttribfv)(GLuint index, GLenum pname, GLfloat* params);
    void        (*glGetVertexAttribiv)(GLuint index, GLenum pname, GLint* params);
    void        (*glGetVertexAttribPointerv)(GLuint index, GLenum pname, GLvoid** pointer);
    void        (*glHint)(GLenum target, GLenum mode);
    GLboolean   (*glIsBuffer)(GLuint buffer);
    GLboolean   (*glIsEnabled)(GLenum cap);
    GLboolean   (*glIsFramebuffer)(GLuint framebuffer);
    GLboolean   (*glIsProgram)(GLuint program);
    GLboolean   (*glIsRenderbuffer)(GLuint renderbuffer);
    GLboolean   (*glIsShader)(GLuint shader);
    GLboolean   (*glIsTexture)(GLuint texture);
    void        (*glLineWidth)(GLfloat width);
    void        (*glLinkProgram)(GLuint program);
    void        (*glPixelStorei)(GLenum pname, GLint param);
    void        (*glPolygonOffset)(GLfloat factor, GLfloat units);
    void        (*glReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels);
    void        (*glReleaseShaderCompiler)(void);
    void        (*glRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    void        (*glSampleCoverage)(GLclampf value, GLboolean invert);
    void        (*glScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
    void        (*glShaderBinary)(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length);
    void        (*glShaderSource)(GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
    void        (*glStencilFunc)(GLenum func, GLint ref, GLuint mask);
    void        (*glStencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask);
    void        (*glStencilMask)(GLuint mask);
    void        (*glStencilMaskSeparate)(GLenum face, GLuint mask);
    void        (*glStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
    void        (*glStencilOpSeparate)(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
    void        (*glTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
    void        (*glTexParameterf)(GLenum target, GLenum pname, GLfloat param);
    void        (*glTexParameterfv)(GLenum, GLenum, const GLfloat*);
    void        (*glTexParameteri)(GLenum, GLenum, GLint);
    void        (*glTexParameteriv)(GLenum, GLenum, const GLint*);
    void        (*glTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum format, GLenum, const GLvoid*);
    void        (*glUniform1f)(GLint, GLfloat);
    void        (*glUniform1fv)(GLint, GLsizei, const GLfloat*);
    void        (*glUniform1i)(GLint, GLint);
    void        (*glUniform1iv)(GLint, GLsizei count, const GLint*);
    void        (*glUniform2f)(GLint, GLfloat, GLfloat);
    void        (*glUniform2fv)(GLint, GLsizei, const GLfloat*);
    void        (*glUniform2i)(GLint, GLint x, GLint);
    void        (*glUniform2iv)(GLint, GLsizei, const GLint*);
    void        (*glUniform3f)(GLint, GLfloat, GLfloat, GLfloat);
    void        (*glUniform3fv)(GLint, GLsizei, const GLfloat*);
    void        (*glUniform3i)(GLint, GLint, GLint, GLint);
    void        (*glUniform3iv)(GLint, GLsizei, const GLint*);
    void        (*glUniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
    void        (*glUniform4fv)(GLint, GLsizei, const GLfloat*);
    void        (*glUniform4i)(GLint, GLint, GLint, GLint, GLint);
    void        (*glUniform4iv)(GLint, GLsizei, const GLint*);
    void        (*glUniformMatrix2fv)(GLint, GLsizei, GLboolean, const GLfloat*);
    void        (*glUniformMatrix3fv)(GLint, GLsizei, GLboolean, const GLfloat*);
    void        (*glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*);
    void        (*glUseProgram)(GLuint);
    void        (*glValidateProgram)(GLuint);
    void        (*glVertexAttrib1f)(GLuint, GLfloat);
    void        (*glVertexAttrib1fv)(GLuint, const GLfloat*);
    void        (*glVertexAttrib2f)(GLuint, GLfloat, GLfloat);
    void        (*glVertexAttrib2fv)(GLuint, const GLfloat*);
    void        (*glVertexAttrib3f)(GLuint, GLfloat, GLfloat, GLfloat);
    void        (*glVertexAttrib3fv)(GLuint, const GLfloat*);
    void        (*glVertexAttrib4f)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
    void        (*glVertexAttrib4fv)(GLuint, const GLfloat*);
    void        (*glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*);
    void        (*glViewport)(GLint, GLint, GLsizei, GLsizei);

#ifdef EGL_KHR_lock_surface
    EGLBoolean (*eglLockSurfaceKHR) (EGLDisplay, EGLSurface, const EGLint *);
    EGLBoolean (*eglUnlockSurfaceKHR) (EGLDisplay, EGLSurface);
#endif

#ifdef EGL_KHR_image
    EGLImageKHR (*eglCreateImageKHR) (EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLint *);
    EGLBoolean (*eglDestroyImageKHR) (EGLDisplay, EGLImageKHR);
#endif

#ifdef EGL_KHR_reusable_sync
    EGLSyncKHR (*eglCreateSyncKHR) (EGLDisplay, EGLenum, const EGLint *);
    EGLBoolean (*eglDestroySyncKHR) (EGLDisplay, EGLSyncKHR);
    EGLint (*eglClientWaitSyncKHR) (EGLDisplay, EGLSyncKHR, EGLint, EGLTimeKHR);
    EGLBoolean (*eglSignalSyncKHR) (EGLDisplay, EGLSyncKHR, EGLenum);
    EGLBoolean (*eglGetSyncAttribKHR) (EGLDisplay, EGLSyncKHR, EGLint, EGLint *);
#endif

#ifdef EGL_NV_sync
    EGLSyncNV (*eglCreateFenceSyncNV) (EGLDisplay, EGLenum, const EGLint *);
    EGLBoolean (*eglDestroySyncNV) (EGLSyncNV);
    EGLBoolean (*eglFenceNV) (EGLSyncNV sync);
    EGLint (*eglClientWaitSyncNV) (EGLSyncNV sync, EGLint flags, EGLTimeNV);
    EGLBoolean (*eglSignalSyncNV) (EGLSyncNV sync, EGLenum mode);
    EGLBoolean (*eglGetSyncAttribNV) (EGLSyncNV sync, EGLint attribute, EGLint *value);
#endif

#ifdef EGL_HI_clientpixmap
    EGLSurface (*eglCreatePixmapSurfaceHI) (EGLDisplay, EGLConfig, struct EGLClientPixmapHI *);
#endif

#ifdef EGL_MESA_drm_image
    EGLImageKHR (*eglCreateDRMImageMESA) (EGLDisplay, const EGLint *);
    EGLBoolean (*eglExportDRMImageMESA) (EGLDisplay, EGLImageKHR, EGLint *, EGLint *, EGLint *);
#endif

#ifdef EGL_POST_SUB_BUFFER_SUPPORTED_NV
    EGLBoolean (*eglPostSubBufferNV) (EGLDisplay, EGLSurface, EGLint, EGLint, EGLint, EGLint);
#endif

#ifdef EGL_SEC_image_map
    void * (*eglMapImageSEC) (EGLDisplay, EGLImageKHR);
    EGLBoolean (*eglUnmapImageSEC) (EGLDisplay, EGLImageKHR);
    EGLBoolean (*eglGetImageAttribSEC) (EGLDisplay, EGLImageKHR, EGLint, EGLint *);
#endif

#ifdef GL_OES_EGL_image
    void (*glEGLImageTargetTexture2DOES) (GLenum, GLeglImageOES);
    void (*glEGLImageTargetRenderbufferStorageOES) (GLenum, GLeglImageOES);
#endif

#ifdef GL_OES_get_program_binary
    void (*glGetProgramBinaryOES) (GLuint, GLsizei, GLsizei *, GLenum *, GLvoid *);
    void (*glProgramBinaryOES) (GLuint, GLenum, const GLvoid *, GLint);
#endif

#ifdef GL_OES_mapbuffer
    void* (*glMapBufferOES) (GLenum, GLenum);
    GLboolean (*glUnmapBufferOES) (GLenum);
    void (*glGetBufferPointervOES) (GLenum, GLenum, GLvoid**);
#endif

#ifdef GL_OES_texture_3D
    void (*glTexImage3DOES) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint,
                           GLenum, GLenum, const GLvoid*);
    void (*glTexSubImage3DOES) (GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei,
                              GLsizei, GLenum, GLenum, const GLvoid*);
    void (*glCopyTexSubImage3DOES) (GLenum, GLint, GLint, GLint, GLint, GLint, GLint,
                                  GLsizei, GLsizei height);
    void (*glCompressedTexImage3DOES) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei,
                                     GLint, GLsizei, const GLvoid*);
    void (*glCompressedTexSubImage3DOES) (GLenum, GLint, GLint, GLint, GLint, GLsizei,
                                        GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
    void (*glFramebufferTexture3DOES) (GLenum, GLenum, GLenum, GLuint, GLint, GLint);
#endif

#ifdef GL_OES_vertex_array_object
    void (*glBindVertexArrayOES) (GLuint);
    void (*glDeleteVertexArraysOES) (GLsizei, const GLuint *);
    void (*glGenVertexArraysOES) (GLsizei, GLuint *);
    GLboolean (*glIsVertexArrayOES) (GLuint);
#endif

#ifdef GL_AMD_performance_monitor
    void (*glGetPerfMonitorGroupsAMD) (GLint *, GLsizei, GLuint *);
    void (*glGetPerfMonitorCountersAMD) (GLuint, GLint *, GLint *, GLsizei, GLuint *);
    void (*glGetPerfMonitorGroupStringAMD) (GLuint, GLsizei, GLsizei *, GLchar *);
    void (*glGetPerfMonitorCounterStringAMD) (GLuint, GLuint, GLsizei, GLsizei *, GLchar *);
    void (*glGetPerfMonitorCounterInfoAMD) (GLuint, GLuint, GLenum, GLvoid *);
    void (*glGenPerfMonitorsAMD) (GLsizei, GLuint *);
    void (*glDeletePerfMonitorsAMD) (GLsizei, GLuint *);
    void (*glSelectPerfMonitorCountersAMD) (GLuint, GLboolean, GLuint, GLint, GLuint *);
    void (*glBeginPerfMonitorAMD) (GLuint);
    void (*glEndPerfMonitorAMD) (GLuint);
    void (*glGetPerfMonitorCounterDataAMD) (GLuint, GLenum, GLsizei, GLuint *, GLint *);
#endif

#ifdef GL_ANGLE_framebuffer_blit
    void (*glBlitFramebufferANGLE) (GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint,
                                  GLbitfield, GLenum);
#endif

#ifdef GL_ANGLE_framebuffer_multisample
    void (*glRenderbufferStorageMultisampleANGLE) (GLenum, GLsizei, GLenum, GLsizei, GLsizei);
#endif

#ifdef GL_APPLE_framebuffer_multisample
    void (*glRenderbufferStorageMultisampleAPPLE) (GLenum, GLsizei, GLenum, GLsizei, GLsizei);
    void (*glResolveMultisampleFramebufferAPPLE) (void);
#endif

#ifdef GL_IMG_multisampled_render_to_texture
    void (*glRenderbufferStorageMultisampleIMG) (GLenum, GLsizei, GLenum, GLsizei, GLsizei);
    void (*glFramebufferTexture2DMultisampleIMG) (GLenum, GLenum, GLenum, GLuint, GLint, GLsizei);
#endif

#ifdef GL_EXT_discard_framebuffer
    void (*glDiscardFramebufferEXT) (GLenum, GLsizei, const GLenum *);
#endif

#ifdef GL_EXT_multisampled_render_to_texture
    void (*glRenderbufferStorageMultisampleEXT) (GLenum, GLsizei, GLenum, GLsizei, GLsizei);
    void (*glFramebufferTexture2DMultisampleEXT) (GLenum, GLenum, GLenum, GLuint, GLint, GLsizei);
#endif

#ifdef GL_EXT_multi_draw_arrays
    void (*glMultiDrawArraysEXT) (GLenum, const GLint *, const GLsizei *, GLsizei);
    void (*glMultiDrawElementsEXT) (GLenum, const GLsizei *, GLenum, const GLvoid **, GLsizei);
#endif

#ifdef GL_NV_fence
    void (*glDeleteFencesNV) (GLsizei, const GLuint *);
    void (*glGenFencesNV) (GLsizei, GLuint *);
    GLboolean (*glIsFenceNV) (GLuint);
    GLboolean (*glTestFenceNV) (GLuint);
    void (*glGetFenceivNV) (GLuint, GLenum, GLint *);
    void (*glFinishFenceNV) (GLuint);
    void (*glSetFenceNV) (GLuint, GLenum);
#endif

#ifdef GL_NV_coverage_sample
    void (*glCoverageMaskNV) (GLboolean mask);
    void (*glCoverageOperationNV) (GLenum operation);
#endif

#ifdef GL_QCOM_driver_control
    void (*glGetDriverControlsQCOM) (GLint *num, GLsizei size, GLuint *driverControls);
    void (*glGetDriverControlStringQCOM) (GLuint driverControl, GLsizei bufSize, GLsizei *length, GLchar *driverControlString);
    void (*glEnableDriverControlQCOM) (GLuint driverControl);
    void (*glDisableDriverControlQCOM) (GLuint driverControl);
#endif

#ifdef GL_QCOM_extended_get
    void (*glExtGetTexturesQCOM) (GLuint *, GLint, GLint *);
    void (*glExtGetBuffersQCOM) (GLuint *, GLint, GLint *);
    void (*glExtGetRenderbuffersQCOM) (GLuint *, GLint, GLint *);
    void (*glExtGetFramebuffersQCOM) (GLuint *, GLint, GLint *);
    void (*glExtGetTexLevelParameterivQCOM) (GLuint, GLenum, GLint, GLenum, GLint *);
    void (*glExtTexObjectStateOverrideiQCOM) (GLenum, GLenum, GLint);
    void (*glExtGetTexSubImageQCOM) (GLenum, GLint, GLint, GLint, GLint, GLsizei,
                                   GLsizei, GLsizei, GLenum, GLenum, GLvoid *);
    void (*glExtGetBufferPointervQCOM) (GLenum, GLvoid **);
#endif

#ifdef GL_QCOM_extended_get2
    void (*glExtGetShadersQCOM) (GLuint *, GLint, GLint *);
    void (*glExtGetProgramsQCOM) (GLuint *, GLint, GLint *);
    GLboolean (*glExtIsProgramBinaryQCOM) (GLuint);
    void (*glExtGetProgramBinarySourceQCOM) (GLuint, GLenum, GLchar *, GLint *);
#endif

#ifdef GL_QCOM_tiled_rendering
    void (*glStartTilingQCOM) (GLuint, GLuint, GLuint, GLuint, GLbitfield);
    void (*glEndTilingQCOM) (GLbitfield);
#endif
} dispatch_t;

#endif /* HAS_GLES2 */

#endif /* GPUPROCESS_DISPATCH_PRIVATE_GLES2_H */
