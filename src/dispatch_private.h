#ifndef GPUPROCESS_DISPATCH_PRIVATE_H
#define GPUPROCESS_DISPATCH_PRIVATE_H

#include "config.h"

#include "compiler_private.h"

#ifdef HAS_GL
#include <GL/glx.h>
#endif

#ifdef HAS_GLES2
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

typedef void (*FunctionPointerType)(void);

typedef struct _dispatch {
#ifdef HAS_GL
/* GLX */
    XVisualInfo* (*glXChooseVisual) (Display *dpy, int screen,
                                  int *attribList);
    GLXContext (*glXCreateContext) (Display *dpy, XVisualInfo *vis,
                                 GLXContext shareList, Bool direct);
    void (*glXDestroyContext) (Display *dpy, GLXContext ctx);
    Bool (*glXMakeCurrent) (Display *dpy, GLXDrawable drawable,
                         GLXContext ctx);
    void (*glXCopyContext) (Display *dpy, GLXContext src, GLXContext dst,
                         unsigned long mask);
    void (*glXSwapBuffers) (Display *dpy, GLXDrawable drawable);
    GLXPixmap (*glXCreateGLXPixmap) (Display *dpy, XVisualInfo *visual,
                                  Pixmap pixmap);
    void (*glXDestroyGLXPixmap) (Display *dpy, GLXPixmap pixmap);
    Bool (*glXQueryExtension) (Display *dpy, int *errorb, int *event);
    Bool (*glXQueryVersion) (Display *dpy, int *maj, int *min);
    Bool (*glXIsDirect) (Display *dpy, GLXContext ctx);
    int (*glXGetConfig) (Display *dpy, XVisualInfo *visual,
                      int attrib, int *value);
    GLXContext (*glXGetCurrentContext) (void);
    GLXDrawable (*glXGetCurrentDrawable) (void);
    void (*glXWaitGL) (void);
    void (*glXWaitX) (void);
    void (*glXUseXFont) (Font font, int first, int count, int list);

    FunctionPointerType (*glXGetProcAddress) (const char *procname);

/* GLX 1.1 and later */
    const char * (*glXQueryExtensionsString) (Display *dpy, int screen);
    const char * (*glXQueryServerString) (Display *dpy, int screen, int name);
    const char * (*glXGetClientString) (Display *dpy, int name);

/* GLX 1.2 and later */
    Display * (*glXGetCurrentDisplay) (void);


/* GLX 1.3 and later */
    GLXFBConfig * (*glXChooseFBConfig) (Display *dpy, int screen,
                                     const int *attribList, int *nitems);
    int (*glXGetFBConfigAttrib) (Display *dpy, GLXFBConfig config,
                              int attribute, int *value);
    GLXFBConfig * (*glXGetFBConfigs) (Display *dpy, int screen,
                                   int *nelements);
    XVisualInfo * (*glXGetVisualFromFBConfig) (Display *dpy,
                                            GLXFBConfig config);
    GLXWindow (*glXCreateWindow) (Display *dpy, GLXFBConfig config,
                               Window win, const int *attribList);
    void (*glXDestroyWindow) (Display *dpy, GLXWindow window);
    GLXPixmap (*glXCreatePixmap) (Display *dpy, GLXFBConfig config,
                               Pixmap pixmap, const int *attribList);
    void (*glXDestroyPixmap) (Display *dpy, GLXPixmap pixmap);
    GLXPbuffer (*glXCreatePbuffer) (Display *dpy, GLXFBConfig config,
                                 const int *attribList);
    void (*glXDestroyPbuffer) (Display *dpy, GLXPbuffer pbuf);
    void (*glXQueryDrawable) (Display *dpy, GLXDrawable draw, int attribute,
                           unsigned int *value);
    GLXContext (*glXCreateNewContext) (Display *dpy, GLXFBConfig config,
                                    int renderType, GLXContext shareList,
                                    Bool direct);
    Bool (*glXMakeContextCurrent) (Display *dpy, GLXDrawable draw,
                                GLXDrawable read, GLXContext ctx);
    GLXDrawable (*glXGetCurrentReadDrawable) (void);
    int (*glXQueryContext) (Display *dpy, GLXContext ctx, int attribute,
                         int *value);
    void (*glXSelectEvent) (Display *dpy, GLXDrawable drawable,
                         unsigned long mask);
    void (*glXGetSelectedEvent) (Display *dpy, GLXDrawable drawable,
                              unsigned long *mask);

#ifdef GLX_EXT_texture_from_pixmap
    void (*glXBindTexImageEXT) (Display *display, GLXDrawable drawable, int buffer, const int *attrib_list);
    void (*glXReleaseTexImageEXT) (Display *display, GLXDrawable drawable, int buffer);
#endif

#endif // HAS_GL

#ifdef HAS_GLES2
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

#endif // HAS_GLES2

#if defined(HAS_GL) || defined(HAS_GLES2)
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
#endif

#if HAS_GL
    void  (*glNewList)(GLuint, GLenum);
    void  (*glEndList)(void);
    void  (*glCallList)(GLuint);
    void  (*glCallLists)(GLsizei, GLenum, const GLvoid *);
    void  (*glDeleteLists)(GLuint, GLsizei);
    GLuint  (*glGenLists)(GLsizei);
    void  (*glListBase)(GLuint);
    void  (*glBegin)(GLenum);
    void  (*glBitmap)(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *);
    void  (*glColor3b)(GLbyte, GLbyte, GLbyte);
    void  (*glColor3bv)(const GLbyte *);
    void  (*glColor3d)(GLdouble, GLdouble, GLdouble);
    void  (*glColor3dv)(const GLdouble *);
    void  (*glColor3f)(GLfloat, GLfloat, GLfloat);
    void  (*glColor3fv)(const GLfloat *);
    void  (*glColor3i)(GLint, GLint, GLint);
    void  (*glColor3iv)(const GLint *);
    void  (*glColor3s)(GLshort, GLshort, GLshort);
    void  (*glColor3sv)(const GLshort *);
    void  (*glColor3ub)(GLubyte, GLubyte, GLubyte);
    void  (*glColor3ubv)(const GLubyte *);
    void  (*glColor3ui)(GLuint, GLuint, GLuint);
    void  (*glColor3uiv)(const GLuint *);
    void  (*glColor3us)(GLushort, GLushort, GLushort);
    void  (*glColor3usv)(const GLushort *);
    void  (*glColor4b)(GLbyte, GLbyte, GLbyte, GLbyte);
    void  (*glColor4bv)(const GLbyte *);
    void  (*glColor4d)(GLdouble, GLdouble, GLdouble, GLdouble);
    void  (*glColor4dv)(const GLdouble *);
    void  (*glColor4f)(GLfloat, GLfloat, GLfloat, GLfloat);
    void  (*glColor4fv)(const GLfloat *);
    void  (*glColor4i)(GLint, GLint, GLint, GLint);
    void  (*glColor4iv)(const GLint *);
    void  (*glColor4s)(GLshort, GLshort, GLshort, GLshort);
    void  (*glColor4sv)(const GLshort *);
    void  (*glColor4ub)(GLubyte, GLubyte, GLubyte, GLubyte);
    void  (*glColor4ubv)(const GLubyte *);
    void  (*glColor4ui)(GLuint, GLuint, GLuint, GLuint);
    void  (*glColor4uiv)(const GLuint *);
    void  (*glColor4us)(GLushort, GLushort, GLushort, GLushort);
    void  (*glColor4usv)(const GLushort *);
    void  (*glEdgeFlag)(GLboolean);
    void  (*glEdgeFlagv)(const GLboolean *);
    void  (*glEnd)(void);
    void  (*glIndexd)(GLdouble);
    void  (*glIndexdv)(const GLdouble *);
    void  (*glIndexf)(GLfloat);
    void  (*glIndexfv)(const GLfloat *);
    void  (*glIndexi)(GLint);
    void  (*glIndexiv)(const GLint *);
    void  (*glIndexs)(GLshort);
    void  (*glIndexsv)(const GLshort *);
    void  (*glNormal3b)(GLbyte, GLbyte, GLbyte);
    void  (*glNormal3bv)(const GLbyte *);
    void  (*glNormal3d)(GLdouble, GLdouble, GLdouble);
    void  (*glNormal3dv)(const GLdouble *);
    void  (*glNormal3f)(GLfloat, GLfloat, GLfloat);
    void  (*glNormal3fv)(const GLfloat *);
    void  (*glNormal3i)(GLint, GLint, GLint);
    void  (*glNormal3iv)(const GLint *);
    void  (*glNormal3s)(GLshort, GLshort, GLshort);
    void  (*glNormal3sv)(const GLshort *);
    void  (*glRasterPos2d)(GLdouble, GLdouble);
    void  (*glRasterPos2dv)(const GLdouble *);
    void  (*glRasterPos2f)(GLfloat, GLfloat);
    void  (*glRasterPos2fv)(const GLfloat *);
    void  (*glRasterPos2i)(GLint, GLint);
    void  (*glRasterPos2iv)(const GLint *);
    void  (*glRasterPos2s)(GLshort, GLshort);
    void  (*glRasterPos2sv)(const GLshort *);
    void  (*glRasterPos3d)(GLdouble, GLdouble, GLdouble);
    void  (*glRasterPos3dv)(const GLdouble *);
    void  (*glRasterPos3f)(GLfloat, GLfloat, GLfloat);
    void  (*glRasterPos3fv)(const GLfloat *);
    void  (*glRasterPos3i)(GLint, GLint, GLint);
    void  (*glRasterPos3iv)(const GLint *);
    void  (*glRasterPos3s)(GLshort, GLshort, GLshort);
    void  (*glRasterPos3sv)(const GLshort *);
    void  (*glRasterPos4d)(GLdouble, GLdouble, GLdouble, GLdouble);
    void  (*glRasterPos4dv)(const GLdouble *);
    void  (*glRasterPos4f)(GLfloat, GLfloat, GLfloat, GLfloat);
    void  (*glRasterPos4fv)(const GLfloat *);
    void  (*glRasterPos4i)(GLint, GLint, GLint, GLint);
    void  (*glRasterPos4iv)(const GLint *);
    void  (*glRasterPos4s)(GLshort, GLshort, GLshort, GLshort);
    void  (*glRasterPos4sv)(const GLshort *);
    void  (*glRectd)(GLdouble, GLdouble, GLdouble, GLdouble);
    void  (*glRectdv)(const GLdouble *, const GLdouble *);
    void  (*glRectf)(GLfloat, GLfloat, GLfloat, GLfloat);
    void  (*glRectfv)(const GLfloat *, const GLfloat *);
    void  (*glRecti)(GLint, GLint, GLint, GLint);
    void  (*glRectiv)(const GLint *, const GLint *);
    void  (*glRects)(GLshort, GLshort, GLshort, GLshort);
    void  (*glRectsv)(const GLshort *, const GLshort *);
    void  (*glTexCoord1d)(GLdouble);
    void  (*glTexCoord1dv)(const GLdouble *);
    void  (*glTexCoord1f)(GLfloat);
    void  (*glTexCoord1fv)(const GLfloat *);
    void  (*glTexCoord1i)(GLint);
    void  (*glTexCoord1iv)(const GLint *);
    void  (*glTexCoord1s)(GLshort);
    void  (*glTexCoord1sv)(const GLshort *);
    void  (*glTexCoord2d)(GLdouble, GLdouble);
    void  (*glTexCoord2dv)(const GLdouble *);
    void  (*glTexCoord2f)(GLfloat, GLfloat);
    void  (*glTexCoord2fv)(const GLfloat *);
    void  (*glTexCoord2i)(GLint, GLint);
    void  (*glTexCoord2iv)(const GLint *);
    void  (*glTexCoord2s)(GLshort, GLshort);
    void  (*glTexCoord2sv)(const GLshort *);
    void  (*glTexCoord3d)(GLdouble, GLdouble, GLdouble);
    void  (*glTexCoord3dv)(const GLdouble *);
    void  (*glTexCoord3f)(GLfloat, GLfloat, GLfloat);
    void  (*glTexCoord3fv)(const GLfloat *);
    void  (*glTexCoord3i)(GLint, GLint, GLint);
    void  (*glTexCoord3iv)(const GLint *);
    void  (*glTexCoord3s)(GLshort, GLshort, GLshort);
    void  (*glTexCoord3sv)(const GLshort *);
    void  (*glTexCoord4d)(GLdouble, GLdouble, GLdouble, GLdouble);
    void  (*glTexCoord4dv)(const GLdouble *);
    void  (*glTexCoord4f)(GLfloat, GLfloat, GLfloat, GLfloat);
    void  (*glTexCoord4fv)(const GLfloat *);
    void  (*glTexCoord4i)(GLint, GLint, GLint, GLint);
    void  (*glTexCoord4iv)(const GLint *);
    void  (*glTexCoord4s)(GLshort, GLshort, GLshort, GLshort);
    void  (*glTexCoord4sv)(const GLshort *);
    void  (*glVertex2d)(GLdouble, GLdouble);
    void  (*glVertex2dv)(const GLdouble *);
    void  (*glVertex2f)(GLfloat, GLfloat);
    void  (*glVertex2fv)(const GLfloat *);
    void  (*glVertex2i)(GLint, GLint);
    void  (*glVertex2iv)(const GLint *);
    void  (*glVertex2s)(GLshort, GLshort);
    void  (*glVertex2sv)(const GLshort *);
    void  (*glVertex3d)(GLdouble, GLdouble, GLdouble);
    void  (*glVertex3dv)(const GLdouble *);
    void  (*glVertex3f)(GLfloat, GLfloat, GLfloat);
    void  (*glVertex3fv)(const GLfloat *);
    void  (*glVertex3i)(GLint, GLint, GLint);
    void  (*glVertex3iv)(const GLint *);
    void  (*glVertex3s)(GLshort, GLshort, GLshort);
    void  (*glVertex3sv)(const GLshort *);
    void  (*glVertex4d)(GLdouble, GLdouble, GLdouble, GLdouble);
    void  (*glVertex4dv)(const GLdouble *);
    void  (*glVertex4f)(GLfloat, GLfloat, GLfloat, GLfloat);
    void  (*glVertex4fv)(const GLfloat *);
    void  (*glVertex4i)(GLint, GLint, GLint, GLint);
    void  (*glVertex4iv)(const GLint *);
    void  (*glVertex4s)(GLshort, GLshort, GLshort, GLshort);
    void  (*glVertex4sv)(const GLshort *);
    void  (*glClipPlane)(GLenum, const GLdouble *);
    void  (*glColorMaterial)(GLenum, GLenum);
    void  (*glFogf)(GLenum, GLfloat);
    void  (*glFogfv)(GLenum, const GLfloat *);
    void  (*glFogi)(GLenum, GLint);
    void  (*glFogiv)(GLenum, const GLint *);
    void  (*glLightf)(GLenum, GLenum, GLfloat);
    void  (*glLightfv)(GLenum, GLenum, const GLfloat *);
    void  (*glLighti)(GLenum, GLenum, GLint);
    void  (*glLightiv)(GLenum, GLenum, const GLint *);
    void  (*glLightModelf)(GLenum, GLfloat);
    void  (*glLightModelfv)(GLenum, const GLfloat *);
    void  (*glLightModeli)(GLenum, GLint);
    void  (*glLightModeliv)(GLenum, const GLint *);
    void  (*glLineStipple)(GLint, GLushort);
    void  (*glMaterialf)(GLenum, GLenum, GLfloat);
    void  (*glMaterialfv)(GLenum, GLenum, const GLfloat *);
    void  (*glMateriali)(GLenum, GLenum, GLint);
    void  (*glMaterialiv)(GLenum, GLenum, const GLint *);
    void  (*glPointSize)(GLfloat);
    void  (*glPolygonMode)(GLenum, GLenum);
    void  (*glPolygonStipple)(const GLubyte *);
    void  (*glShadeModel)(GLenum);
    void  (*glTexImage1D)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    void  (*glTexEnvf)(GLenum, GLenum, GLfloat);
    void  (*glTexEnvfv)(GLenum, GLenum, const GLfloat *);
    void  (*glTexEnvi)(GLenum, GLenum, GLint);
    void  (*glTexEnviv)(GLenum, GLenum, const GLint *);
    void  (*glTexGend)(GLenum, GLenum, GLdouble);
    void  (*glTexGendv)(GLenum, GLenum, const GLdouble *);
    void  (*glTexGenf)(GLenum, GLenum, GLfloat);
    void  (*glTexGenfv)(GLenum, GLenum, const GLfloat *);
    void  (*glTexGeni)(GLenum, GLenum, GLint);
    void  (*glTexGeniv)(GLenum, GLenum, const GLint *);
    void  (*glFeedbackBuffer)(GLsizei, GLenum, GLfloat *);
    void  (*glSelectBuffer)(GLsizei, GLuint *);
    GLint (*glRenderMode)(GLenum);
    void  (*glInitNames)(void);
    void  (*glLoadName)(GLuint);
    void  (*glPassThrough)(GLfloat);
    void  (*glPopName)(void);
    void  (*glPushName)(GLuint);
    void  (*glDrawBuffer)(GLenum);
    void  (*glClearAccum)(GLfloat, GLfloat, GLfloat, GLfloat);
    void  (*glClearIndex)(GLfloat);
    void  (*glClearDepth)(GLclampd);
    void  (*glIndexMask)(GLuint);
    void  (*glAccum)(GLenum, GLfloat);
    void  (*glPopAttrib)(void);
    void  (*glPushAttrib)(GLbitfield);
    void  (*glMap1d)(GLenum, GLdouble, GLdouble, GLint, GLint, const GLdouble *);
    void  (*glMap1f)(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat *);
    void  (*glMap2d)(GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, const GLdouble *);
    void  (*glMap2f)(GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, const GLfloat *);
    void  (*glMapGrid1d)(GLint, GLdouble, GLdouble);
    void  (*glMapGrid1f)(GLint, GLfloat, GLfloat);
    void  (*glMapGrid2d)(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble);
    void  (*glMapGrid2f)(GLint, GLfloat, GLfloat, GLint, GLfloat, GLfloat);
    void  (*glEvalCoord1d)(GLdouble);
    void  (*glEvalCoord1dv)(const GLdouble *);
    void  (*glEvalCoord1f)(GLfloat);
    void  (*glEvalCoord1fv)(const GLfloat *);
    void  (*glEvalCoord2d)(GLdouble u, GLdouble v);
    void  (*glEvalCoord2dv)(const GLdouble *);
    void  (*glEvalCoord2f)(GLfloat, GLfloat);
    void  (*glEvalCoord2fv)(const GLfloat *);
    void  (*glEvalMesh1)(GLenum, GLint, GLint);
    void  (*glEvalPoint1)(GLint);
    void  (*glEvalMesh2)(GLenum, GLint, GLint, GLint, GLint);
    void  (*glEvalPoint2)(GLint, GLint);
    void  (*glAlphaFunc)(GLenum, GLclampf);
    void  (*glLogicOp)(GLenum);
    void  (*glPixelZoom)(GLfloat, GLfloat);
    void  (*glPixelTransferf)(GLenum, GLfloat);
    void  (*glPixelTransferi)(GLenum, GLint);
    void  (*glPixelStoref)(GLenum, GLfloat);
    void  (*glPixelMapfv)(GLenum, GLsizei, const GLfloat *);
    void  (*glPixelMapuiv)(GLenum, GLsizei, const GLuint *);
    void  (*glPixelMapusv)(GLenum, GLsizei, const GLushort *);
    void  (*glReadBuffer)(GLenum);
    void  (*glCopyPixels)(GLint, GLint, GLsizei, GLsizei, GLenum);
    void  (*glDrawPixels)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    void  (*glGetClipPlane)(GLenum, GLdouble *);
    void  (*glGetDoublev)(GLenum, GLdouble *);
    void  (*glGetLightfv)(GLenum, GLenum, GLfloat *);
    void  (*glGetLightiv)(GLenum, GLenum, GLint *);
    void  (*glGetMapdv)(GLenum, GLenum, GLdouble *);
    void  (*glGetMapfv)(GLenum, GLenum, GLfloat *);
    void  (*glGetMapiv)(GLenum, GLenum, GLint *);
    void  (*glGetMaterialfv)(GLenum, GLenum, GLfloat *);
    void  (*glGetMaterialiv)(GLenum, GLenum, GLint *);
    void  (*glGetPixelMapfv)(GLenum, GLfloat *);
    void  (*glGetPixelMapuiv)(GLenum, GLuint *);
    void  (*glGetPixelMapusv)(GLenum, GLushort *);
    void  (*glGetPolygonStipple)(GLubyte *);
    void  (*glGetTexEnvfv)(GLenum, GLenum, GLfloat *);
    void  (*glGetTexEnviv)(GLenum, GLenum, GLint *);
    void  (*glGetTexGendv)(GLenum, GLenum, GLdouble *);
    void  (*glGetTexGenfv)(GLenum, GLenum, GLfloat *);
    void  (*glGetTexGeniv)(GLenum, GLenum, GLint *);
    void  (*glGetTexImage)(GLenum, GLint, GLenum, GLenum, GLvoid *);
    void  (*glGetTexLevelParameterfv)(GLenum, GLint, GLenum, GLfloat *);
    void  (*glGetTexLevelParameteriv)(GLenum, GLint, GLenum, GLint *);
    GLboolean  (*glIsList)(GLuint);
    void  (*glDepthRange)(GLclampd, GLclampd);
    void  (*glFrustum)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
    void  (*glLoadIdentity)(void);
    void  (*glLoadMatrixf)(const GLfloat *);
    void  (*glLoadMatrixd)(const GLdouble *);
    void  (*glMatrixMode)(GLenum);
    void  (*glMultMatrixf)(const GLfloat *);
    void  (*glMultMatrixd)(const GLdouble *);
    void  (*glOrtho)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
    void  (*glPopMatrix)(void);
    void  (*glPushMatrix)(void);
    void  (*glRotated)(GLdouble, GLdouble, GLdouble, GLdouble);
    void  (*glRotatef)(GLfloat, GLfloat, GLfloat, GLfloat);
    void  (*glScaled)( GLdouble, GLdouble y, GLdouble z);
    void  (*glScalef)(GLfloat, GLfloat, GLfloat);
    void  (*glTranslated)(GLdouble, GLdouble, GLdouble);
    void  (*glTranslatef)(GLfloat, GLfloat, GLfloat);
    void  (*glArrayElement)(GLint);
    void  (*glColorPointer)(GLint, GLenum, GLsizei, const GLvoid *);
    void  (*glDisableClientState)(GLenum);
    void  (*glEdgeFlagPointer)(GLsizei, const GLvoid *);
    void  (*glEnableClientState)(GLenum);
    void  (*glIndexPointer)(GLenum, GLsizei, const GLvoid *);
    void  (*glIndexub)(GLubyte);
    void  (*glIndexubv)(const GLubyte *);
    void  (*glInterleavedArrays)(GLenum, GLsizei, const GLvoid *);
    void  (*glNormalPointer)(GLenum, GLsizei, const GLvoid *);
    void  (*glTexCoordPointer)(GLint, GLenum, GLsizei, const GLvoid *);
    void  (*glVertexPointer)(GLint, GLenum, GLsizei, const GLvoid *);
    GLboolean  (*glAreTexturesResident)(GLsizei, const GLuint *, GLboolean *);
    void  (*glCopyTexImage1D)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint);
    void  (*glCopyTexSubImage1D)(GLenum, GLint, GLint, GLint, GLint, GLsizei);
    void  (*glGetPointerv)(GLenum, GLvoid **);
    void  (*glPrioritizeTextures)(GLsizei, const GLuint *, const GLclampf *);
    void  (*glTexSubImage1D)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *);
    void  (*glPopClientAttrib)(void);
    void  (*glPushClientAttrib)(GLbitfield);
    void  (*glDrawRangeElements)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *);
    void  (*glColorTable)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
    void  (*glColorTableParameterfv)(GLenum, GLenum, const GLfloat *);
    void  (*glColorTableParameteriv)(GLenum, GLenum, const GLint *);
    void  (*glCopyColorTable)(GLenum, GLenum, GLint, GLint, GLsizei);
    void  (*glGetColorTable)(GLenum, GLenum, GLenum, GLvoid *);
    void  (*glGetColorTableParameterfv)(GLenum, GLenum, GLfloat *);
    void  (*glGetColorTableParameteriv)(GLenum, GLenum, GLint *);
    void  (*glColorSubTable)(GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    void  (*glCopyColorSubTable)(GLenum, GLsizei, GLint, GLint, GLsizei);
    void  (*glConvolutionFilter1D)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
    void  (*glConvolutionFilter2D)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    void  (*glConvolutionParameterf)(GLenum, GLenum, GLfloat);
    void  (*glConvolutionParameterfv)(GLenum, GLenum, const GLfloat *);
    void  (*glConvolutionParameteri)(GLenum, GLenum, GLint);
    void  (*glConvolutionParameteriv)(GLenum, GLenum, const GLint *);
    void  (*glCopyConvolutionFilter1D)(GLenum, GLenum, GLint, GLint, GLsizei);
    void  (*glCopyConvolutionFilter2D)(GLenum, GLenum, GLint, GLint, GLsizei, GLsizei);
    void  (*glGetConvolutionFilter)(GLenum, GLenum, GLenum, GLvoid *);
    void  (*glGetConvolutionParameterfv)(GLenum, GLenum, GLfloat *);
    void  (*glGetConvolutionParameteriv)(GLenum, GLenum, GLint *);
    void  (*glGetSeparableFilter)(GLenum, GLenum, GLenum, GLvoid *, GLvoid *, GLvoid *);
    void  (*glSeparableFilter2D)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *, const GLvoid *);
    void  (*glGetHistogram)(GLenum, GLboolean, GLenum, GLenum, GLvoid *);
    void  (*glGetHistogramParameterfv)(GLenum, GLenum, GLfloat *);
    void  (*glGetHistogramParameteriv)(GLenum, GLenum, GLint *);
    void  (*glGetMinmax)(GLenum, GLboolean, GLenum, GLenum, GLvoid *);
    void  (*glGetMinmaxParameterfv)(GLenum, GLenum, GLfloat *);
    void  (*glGetMinmaxParameteriv)(GLenum, GLenum, GLint *);
    void  (*glHistogram)(GLenum, GLsizei, GLenum, GLboolean);
    void  (*glMinmax)(GLenum, GLenum, GLboolean);
    void  (*glResetHistogram)(GLenum);
    void  (*glResetMinmax)(GLenum);
    void  (*glTexImage3D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    void  (*glTexSubImage3D)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    void  (*glCopyTexSubImage3D)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
    void  (*glClientActiveTextureARB)(GLenum);
    void  (*glMultiTexCoord1dARB)(GLenum, GLdouble);
    void  (*glMultiTexCoord1dvARB)(GLenum, const GLdouble *);
    void  (*glMultiTexCoord1fARB)(GLenum, GLfloat);
    void  (*glMultiTexCoord1fvARB)(GLenum, const GLfloat *);
    void  (*glMultiTexCoord1iARB)(GLenum, GLint);
    void  (*glMultiTexCoord1ivARB)(GLenum, const GLint *);
    void  (*glMultiTexCoord1sARB)(GLenum, GLshort);
    void  (*glMultiTexCoord1svARB)(GLenum, const GLshort *);
    void  (*glMultiTexCoord2dARB)(GLenum, GLdouble, GLdouble);
    void  (*glMultiTexCoord2dvARB)(GLenum, const GLdouble *);
    void  (*glMultiTexCoord2fARB)(GLenum, GLfloat, GLfloat);
    void  (*glMultiTexCoord2fvARB)(GLenum, const GLfloat *);
    void  (*glMultiTexCoord2iARB)(GLenum, GLint, GLint);
    void  (*glMultiTexCoord2ivARB)(GLenum, const GLint *);
    void  (*glMultiTexCoord2sARB)(GLenum, GLshort, GLshort);
    void  (*glMultiTexCoord2svARB)(GLenum, const GLshort *);
    void  (*glMultiTexCoord3dARB)(GLenum, GLdouble, GLdouble, GLdouble);
    void  (*glMultiTexCoord3dvARB)(GLenum, const GLdouble *);
    void  (*glMultiTexCoord3fARB)(GLenum, GLfloat, GLfloat, GLfloat);
    void  (*glMultiTexCoord3fvARB)(GLenum, const GLfloat *);
    void  (*glMultiTexCoord3iARB)(GLenum, GLint, GLint, GLint);
    void  (*glMultiTexCoord3ivARB)(GLenum, const GLint *);
    void  (*glMultiTexCoord3sARB)(GLenum, GLshort, GLshort, GLshort);
    void  (*glMultiTexCoord3svARB)(GLenum, const GLshort *);
    void  (*glMultiTexCoord4dARB)(GLenum, GLdouble, GLdouble, GLdouble, GLdouble);
    void  (*glMultiTexCoord4dvARB)(GLenum, const GLdouble *);
    void  (*glMultiTexCoord4fARB)(GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
    void  (*glMultiTexCoord4fvARB)(GLenum, const GLfloat *);
    void  (*glMultiTexCoord4iARB)(GLenum, GLint, GLint, GLint, GLint);
    void  (*glMultiTexCoord4ivARB)(GLenum, const GLint *);
    void  (*glMultiTexCoord4sARB)(GLenum, GLshort, GLshort, GLshort, GLshort);
    void  (*glMultiTexCoord4svARB)(GLenum, const GLshort *);
    void  (*glUniformMatrix2x3fv)(GLint, GLsizei, GLboolean, const GLfloat *);
    void  (*glUniformMatrix2x4fv)(GLint, GLsizei, GLboolean, const GLfloat *);
    void  (*glUniformMatrix3x2fv)(GLint, GLsizei, GLboolean, const GLfloat *);
    void  (*glUniformMatrix3x4fv)(GLint, GLsizei, GLboolean, const GLfloat *);
    void  (*glUniformMatrix4x2fv)(GLint, GLsizei, GLboolean, const GLfloat *);
    void  (*glUniformMatrix4x3fv)(GLint, GLsizei, GLboolean, const GLfloat *);
    void  (*glClampColor)(GLenum, GLenum);
    void  (*glClearBufferfi)(GLenum, GLint, GLfloat, GLint);
    void  (*glClearBufferfv)(GLenum, GLint, const GLfloat *);
    void  (*glClearBufferiv)(GLenum, GLint, const GLint *);
    void  (*glClearBufferuiv)(GLenum, GLint, const GLuint *);
    const GLubyte *  (*glGetStringi)(GLenum, GLuint);
    void  (*glFramebufferTexture)(GLenum, GLenum, GLuint, GLint);
    void  (*glGetBufferParameteri64v)(GLenum, GLenum, GLint64 *);
    void  (*glGetInteger64i_v)(GLenum, GLuint, GLint64 *);
    void  (*glVertexAttribDivisor)(GLuint, GLuint);
    void  (*glLoadTransposeMatrixdARB)(const GLdouble *);
    void  (*glLoadTransposeMatrixfARB)(const GLfloat *);
    void  (*glMultTransposeMatrixdARB)(const GLdouble *);
    void  (*glMultTransposeMatrixfARB)(const GLfloat *);
    void  (*glSampleCoverageARB)(GLclampf, GLboolean);
    void  (*glCompressedTexImage1DARB)(GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid *);
    void  (*glCompressedTexImage3DARB)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *);
    void  (*glCompressedTexSubImage1DARB)(GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid *);
    void  (*glCompressedTexSubImage3DARB)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *);
    void  (*glGetCompressedTexImageARB)(GLenum, GLint, GLvoid *);
    void  (*glDisableVertexAttribArrayARB)(GLuint);
    void  (*glEnableVertexAttribArrayARB)(GLuint);
    void  (*glGetProgramEnvParameterdvARB)(GLenum, GLuint, GLdouble *);
    void  (*glGetProgramEnvParameterfvARB)(GLenum, GLuint, GLfloat *);
    void  (*glGetProgramLocalParameterdvARB)(GLenum, GLuint, GLdouble *);
    void  (*glGetProgramLocalParameterfvARB)(GLenum, GLuint, GLfloat *);
    void  (*glGetProgramStringARB)(GLenum, GLenum, GLvoid *);
    void  (*glGetProgramivARB)(GLenum, GLenum, GLint *);
    void  (*glGetVertexAttribdvARB)(GLuint, GLenum, GLdouble *);
    void  (*glGetVertexAttribfvARB)(GLuint, GLenum, GLfloat *);
    void  (*glGetVertexAttribivARB)(GLuint, GLenum, GLint *);
    void  (*glProgramEnvParameter4dARB)(GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble);
    void  (*glProgramEnvParameter4dvARB)(GLenum, GLuint, const GLdouble *);
    void  (*glProgramEnvParameter4fARB)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
    void  (*glProgramEnvParameter4fvARB)(GLenum, GLuint, const GLfloat *);
    void  (*glProgramLocalParameter4dARB)(GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble);
    void  (*glProgramLocalParameter4dvARB)(GLenum, GLuint, const GLdouble *);
    void  (*glProgramLocalParameter4fARB)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
    void  (*glProgramLocalParameter4fvARB)(GLenum, GLuint, const GLfloat *);
    void  (*glProgramStringARB)(GLenum, GLenum, GLsizei, const GLvoid *);
    void  (*glVertexAttrib1dARB)(GLuint, GLdouble);
    void  (*glVertexAttrib1dvARB)(GLuint, const GLdouble *);
    void  (*glVertexAttrib1fARB)(GLuint, GLfloat);
    void  (*glVertexAttrib1fvARB)(GLuint, const GLfloat *);
    void  (*glVertexAttrib1sARB)(GLuint, GLshort);
    void  (*glVertexAttrib1svARB)(GLuint, const GLshort *);
    void  (*glVertexAttrib2dARB)(GLuint, GLdouble, GLdouble);
    void  (*glVertexAttrib2dvARB)(GLuint, const GLdouble *);
    void  (*glVertexAttrib2fARB)(GLuint, GLfloat, GLfloat);
    void  (*glVertexAttrib2fvARB)(GLuint, const GLfloat *);
    void  (*glVertexAttrib2sARB)(GLuint, GLshort, GLshort);
    void  (*glVertexAttrib2svARB)(GLuint, const GLshort *);
    void  (*glVertexAttrib3dARB)(GLuint, GLdouble, GLdouble, GLdouble);
    void  (*glVertexAttrib3dvARB)(GLuint, const GLdouble *);
    void  (*glVertexAttrib3fARB)(GLuint, GLfloat, GLfloat, GLfloat);
    void  (*glVertexAttrib3fvARB)(GLuint, const GLfloat *);
    void  (*glVertexAttrib3sARB)(GLuint, GLshort, GLshort, GLshort);
    void  (*glVertexAttrib3svARB)(GLuint, const GLshort *);
    void  (*glVertexAttrib4NbvARB)(GLuint, const GLbyte *);
    void  (*glVertexAttrib4NivARB)(GLuint, const GLint *);
    void  (*glVertexAttrib4NsvARB)(GLuint, const GLshort *);
    void  (*glVertexAttrib4NubARB)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte);
    void  (*glVertexAttrib4NubvARB)(GLuint, const GLubyte *);
    void  (*glVertexAttrib4NuivARB)(GLuint, const GLuint *);
    void  (*glVertexAttrib4NusvARB)(GLuint, const GLushort *);
    void  (*glVertexAttrib4bvARB)(GLuint, const GLbyte *);
    void  (*glVertexAttrib4dARB)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble);
    void  (*glVertexAttrib4dvARB)(GLuint, const GLdouble *);
    void  (*glVertexAttrib4fARB)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
    void  (*glVertexAttrib4fvARB)(GLuint, const GLfloat *);
    void  (*glVertexAttrib4ivARB)(GLuint, const GLint *);
    void  (*glVertexAttrib4sARB)(GLuint, GLshort, GLshort, GLshort, GLshort);
    void  (*glVertexAttrib4svARB)(GLuint, const GLshort *);
    void  (*glVertexAttrib4ubvARB)(GLuint, const GLubyte *);
    void  (*glVertexAttrib4uivARB)(GLuint, const GLuint *);
    void  (*glVertexAttrib4usvARB)(GLuint, const GLushort *);
    void  (*glVertexAttribPointerARB)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *);
    void  (*glBufferDataARB)(GLenum, GLsizeiptrARB, const GLvoid *, GLenum);
    void  (*glBufferSubDataARB)(GLenum, GLintptrARB, GLsizeiptrARB, const GLvoid *);
    void  (*glGenBuffersARB)(GLsizei, GLuint *);
    void  (*glGetBufferParameterivARB)(GLenum, GLenum, GLint *);
    void  (*glGetBufferPointervARB)(GLenum, GLenum, GLvoid **);
    void  (*glGetBufferSubDataARB)(GLenum, GLintptrARB, GLsizeiptrARB, GLvoid *);
    GLboolean   (*glIsBufferARB)(GLuint);
    GLvoid *    (*glMapBufferARB)(GLenum, GLenum);
    GLboolean   (*glUnmapBufferARB)(GLenum);
    void  (*glBeginQueryARB)(GLenum, GLuint);
    void  (*glDeleteQueriesARB)(GLsizei, const GLuint *);
    void  (*glEndQueryARB)(GLenum);
    void  (*glGenQueriesARB)(GLsizei, GLuint *);
    void  (*glGetQueryObjectivARB)(GLuint, GLenum, GLint *);
    void  (*glGetQueryObjectuivARB)(GLuint, GLenum, GLuint *);
    void  (*glGetQueryivARB)(GLenum, GLenum, GLint *);
    GLboolean  (*glIsQueryARB)(GLuint);
    void  (*glAttachObjectARB)(GLhandleARB, GLhandleARB);
    void  (*glCompileShaderARB)(GLhandleARB);
    GLhandleARB  (*glCreateProgramObjectARB)(void);
    GLhandleARB  (*glCreateShaderObjectARB)(GLenum);
    void  (*glDeleteObjectARB)(GLhandleARB);
    void  (*glDetachObjectARB)(GLhandleARB, GLhandleARB);
    void  (*glGetActiveUniformARB)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *);
    void  (*glGetAttachedObjectsARB)(GLhandleARB, GLsizei, GLsizei *, GLhandleARB *);
    GLhandleARB  (*glGetHandleARB)(GLenum);
    void  (*glGetInfoLogARB)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *);
    void  (*glGetObjectParameterfvARB)(GLhandleARB, GLenum, GLfloat *);
    void  (*glGetObjectParameterivARB)(GLhandleARB, GLenum, GLint *);
    void  (*glGetShaderSourceARB)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *);
    GLint  (*glGetUniformLocationARB)(GLhandleARB, const GLcharARB *);
    void  (*glGetUniformfvARB)(GLhandleARB, GLint, GLfloat *);
    void  (*glGetUniformivARB)(GLhandleARB, GLint, GLint *);
    void  (*glLinkProgramARB)(GLhandleARB);
    void  (*glShaderSourceARB)(GLhandleARB, GLsizei, const GLcharARB **, const GLint *);
    void  (*glUniform1fARB)(GLint, GLfloat);
    void  (*glUniform1fvARB)(GLint, GLsizei, const GLfloat *);
    void  (*glUniform1iARB)(GLint, GLint);
    void  (*glUniform1ivARB)(GLint, GLsizei, const GLint *);
    void  (*glUniform2fARB)(GLint, GLfloat, GLfloat);
    void  (*glUniform2fvARB)(GLint, GLsizei, const GLfloat *);
    void  (*glUniform2iARB)(GLint, GLint, GLint);
    void  (*glUniform2ivARB)(GLint, GLsizei, const GLint *);
    void  (*glUniform3fARB)(GLint, GLfloat, GLfloat, GLfloat);
    void  (*glUniform3fvARB)(GLint, GLsizei, const GLfloat *);
    void  (*glUniform3iARB)(GLint, GLint, GLint, GLint);
    void  (*glUniform3ivARB)(GLint, GLsizei, const GLint *);
    void  (*glUniform4fARB)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
    void  (*glUniform4fvARB)(GLint, GLsizei, const GLfloat *);
    void  (*glUniform4iARB)(GLint, GLint, GLint, GLint, GLint);
    void  (*glUniform4ivARB)(GLint, GLsizei, const GLint *);
    void  (*glUniformMatrix2fvARB)(GLint, GLsizei, GLboolean, const GLfloat *);
    void  (*glUniformMatrix3fvARB)(GLint, GLsizei, GLboolean, const GLfloat *);
    void  (*glUniformMatrix4fvARB)(GLint, GLsizei, GLboolean, const GLfloat *);
    void  (*glUseProgramObjectARB)(GLhandleARB);
    void  (*glValidateProgramARB)(GLhandleARB);
    void  (*glBindAttribLocationARB)(GLhandleARB, GLuint, const GLcharARB *);
    void  (*glGetActiveAttribARB)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *);
    GLint  (*glGetAttribLocationARB)(GLhandleARB, const GLcharARB *);
    void  (*glDrawBuffersARB)(GLsizei, const GLenum *);
    void  (*glClampColorARB)(GLenum, GLenum);
    void  (*glDrawArraysInstancedARB)(GLenum, GLint, GLsizei, GLsizei);
    void  (*glDrawElementsInstancedARB)(GLenum, GLsizei, GLenum, const GLvoid *, GLsizei);
    void  (*glRenderbufferStorageMultisample)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
    void  (*glFramebufferTextureARB)(GLenum, GLenum, GLuint, GLint);
    void  (*glFramebufferTextureFaceARB)(GLenum, GLenum, GLuint, GLint, GLenum);
    void  (*glProgramParameteriARB)(GLuint, GLenum, GLint);
    void  (*glVertexAttribDivisorARB)(GLuint, GLuint);
    void  (*glFlushMappedBufferRange)(GLenum, GLintptr, GLsizeiptr);
    GLvoid *  (*glMapBufferRange)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
    void  (*glTexBufferARB)(GLenum, GLenum, GLuint);
    void  (*glBindVertexArray)(GLuint);
    void  (*glGenVertexArrays)(GLsizei, GLuint *);
    void  (*glGetActiveUniformBlockName)(GLuint, GLuint, GLsizei, GLsizei *, GLchar *);
    void  (*glGetActiveUniformBlockiv)(GLuint, GLuint, GLenum, GLint *);
    void  (*glGetActiveUniformName)(GLuint, GLuint, GLsizei, GLsizei *, GLchar *);
    void  (*glGetActiveUniformsiv)(GLuint, GLsizei, const GLuint *, GLenum, GLint *);
    GLuint  (*glGetUniformBlockIndex)(GLuint, const GLchar *);
    void  (*glGetUniformIndices)(GLuint, GLsizei, const GLchar * const *, GLuint *);
    void  (*glUniformBlockBinding)(GLuint, GLuint, GLuint);
    void  (*glCopyBufferSubData)(GLenum, GLenum, GLintptr, GLintptr, GLsizeiptr);
    GLenum  (*glClientWaitSync)(GLsync, GLbitfield, GLuint64);
    void    (*glDeleteSync)(GLsync);
    GLsync  (*glFenceSync)(GLenum, GLbitfield);
    void    (*glGetInteger64v)(GLenum, GLint64 *);
    void    (*glGetSynciv)(GLsync, GLenum, GLsizei, GLsizei *, GLint *);
    GLboolean  (*glIsSync)(GLsync);
    void  (*glWaitSync)(GLsync, GLbitfield, GLuint64);
    void  (*glDrawElementsBaseVertex)(GLenum, GLsizei, GLenum, const GLvoid *, GLint);
    void  (*glDrawElementsInstancedBaseVertex)(GLenum, GLsizei, GLenum, const GLvoid *, GLsizei, GLint);
    void  (*glDrawRangeElementsBaseVertex)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *, GLint);
    void  (*glMultiDrawElementsBaseVertex)(GLenum, const GLsizei *, GLenum, const GLvoid * const *, GLsizei, const GLint *);
    void  (*glBlendEquationSeparateiARB)(GLuint, GLenum, GLenum);
    void  (*glBlendEquationiARB)(GLuint, GLenum);
    void  (*glBlendFuncSeparateiARB)(GLuint, GLenum, GLenum, GLenum, GLenum);
    void  (*glBlendFunciARB)(GLuint, GLenum, GLenum);
    void  (*glBindFragDataLocationIndexed)(GLuint, GLuint, GLuint, const GLchar *);
    GLint  (*glGetFragDataIndex)(GLuint, const GLchar *);
    void  (*glBindSampler)(GLuint, GLuint);
    void  (*glDeleteSamplers)(GLsizei, const GLuint *);
    void  (*glGenSamplers)(GLsizei, GLuint *);
    void  (*glGetSamplerParameterIiv)(GLuint, GLenum, GLint *);
    void  (*glGetSamplerParameterIuiv)(GLuint, GLenum, GLuint *);
    void  (*glGetSamplerParameterfv)(GLuint, GLenum, GLfloat *);
    void  (*glGetSamplerParameteriv)(GLuint, GLenum, GLint *);
    GLboolean  (*glIsSampler)(GLuint);
    void  (*glSamplerParameterIiv)(GLuint, GLenum, const GLint *);
    void  (*glSamplerParameterIuiv)(GLuint, GLenum, const GLuint *);
    void  (*glSamplerParameterf)(GLuint, GLenum, GLfloat);
    void  (*glSamplerParameterfv)(GLuint, GLenum, const GLfloat *);
    void  (*glSamplerParameteri)(GLuint, GLenum, GLint);
    void  (*glSamplerParameteriv)(GLuint, GLenum, const GLint *);
    void  (*glQueryCounter)(GLuint, GLenum);
    void  (*glColorP3ui)(GLenum, GLuint);
    void  (*glColorP3uiv)(GLenum, const GLuint *);
    void  (*glColorP4ui)(GLenum, GLuint);
    void  (*glColorP4uiv)(GLenum, const GLuint *);
    void  (*glMultiTexCoordP1ui)(GLenum, GLenum, GLuint);
    void  (*glMultiTexCoordP1uiv)(GLenum, GLenum, const GLuint *);
    void  (*glMultiTexCoordP2ui)(GLenum, GLenum, GLuint);
    void  (*glMultiTexCoordP2uiv)(GLenum, GLenum, const GLuint *);
    void  (*glMultiTexCoordP3ui)(GLenum, GLenum, GLuint);
    void  (*glMultiTexCoordP3uiv)(GLenum, GLenum, const GLuint *);
    void  (*glMultiTexCoordP4ui)(GLenum, GLenum, GLuint);
    void  (*glMultiTexCoordP4uiv)(GLenum, GLenum, const GLuint *);
    void  (*glNormalP3ui)(GLenum, GLuint);
    void  (*glNormalP3uiv)(GLenum, const GLuint *);
    void  (*glSecondaryColorP3ui)(GLenum, GLuint);
    void  (*glSecondaryColorP3uiv)(GLenum, const GLuint *);
    void  (*glTexCoordP1ui)(GLenum, GLuint);
    void  (*glTexCoordP1uiv)(GLenum, const GLuint *);
    void  (*glTexCoordP2ui)(GLenum, GLuint);
    void  (*glTexCoordP2uiv)(GLenum, const GLuint *);
    void  (*glTexCoordP3ui)(GLenum, GLuint);
    void  (*glTexCoordP3uiv)(GLenum, const GLuint *);
    void  (*glTexCoordP4ui)(GLenum, GLuint);
    void  (*glTexCoordP4uiv)(GLenum, const GLuint *);
    void  (*glVertexAttribP1ui)(GLuint, GLenum, GLboolean, GLuint);
    void  (*glVertexAttribP1uiv)(GLuint, GLenum, GLboolean, const GLuint *);
    void  (*glVertexAttribP2ui)(GLuint, GLenum, GLboolean, GLuint);
    void  (*glVertexAttribP2uiv)(GLuint, GLenum, GLboolean, const GLuint *);
    void  (*glVertexAttribP3ui)(GLuint, GLenum, GLboolean, GLuint);
    void  (*glVertexAttribP3uiv)(GLuint, GLenum, GLboolean, const GLuint *);
    void  (*glVertexAttribP4ui)(GLuint, GLenum, GLboolean, GLuint);
    void  (*glVertexAttribP4uiv)(GLuint, GLenum, GLboolean, const GLuint *);
    void  (*glVertexP2ui)(GLenum, GLuint);
    void  (*glVertexP2uiv)(GLenum, const GLuint *);
    void  (*glVertexP3ui)(GLenum, GLuint);
    void  (*glVertexP3uiv)(GLenum, const GLuint *);
    void  (*glVertexP4ui)(GLenum, GLuint);
    void  (*glVertexP4uiv)(GLenum, const GLuint *);
    void  (*glBindTransformFeedback)(GLenum, GLuint);
    void  (*glDeleteTransformFeedbacks)(GLsizei, const GLuint *);
    void  (*glDrawTransformFeedback)(GLenum, GLuint);
    void  (*glGenTransformFeedbacks)(GLsizei, GLuint *);
    GLboolean  (*glIsTransformFeedback)(GLuint);
    void  (*glPauseTransformFeedback)(void);
    void  (*glResumeTransformFeedback)(void);
    void  (*glBeginQueryIndexed)(GLenum, GLuint, GLuint);
    void  (*glDrawTransformFeedbackStream)(GLenum, GLuint, GLuint);
    void  (*glEndQueryIndexed)(GLenum, GLuint);
    void  (*glGetQueryIndexediv)(GLenum, GLuint, GLenum, GLint *);
    void  (*glDebugMessageCallbackARB)(GLDEBUGPROCARB, const GLvoid *);
    void  (*glDebugMessageControlARB)(GLenum, GLenum, GLenum, GLsizei, const GLuint *, GLboolean);
    void  (*glDebugMessageInsertARB)(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLcharARB *);
    GLuint  (*glGetDebugMessageLogARB)(GLuint, GLsizei, GLenum *, GLenum *, GLuint *, GLenum *, GLsizei *, GLcharARB *);
    GLenum  (*glGetGraphicsResetStatusARB)(void);
    void  (*glGetnColorTableARB)(GLenum, GLenum, GLenum, GLsizei, GLvoid *);
    void  (*glGetnCompressedTexImageARB)(GLenum, GLint, GLsizei, GLvoid *);
    void  (*glGetnConvolutionFilterARB)(GLenum, GLenum, GLenum, GLsizei, GLvoid *);
    void  (*glGetnHistogramARB)(GLenum, GLboolean, GLenum, GLenum, GLsizei, GLvoid *);
    void  (*glGetnMapdvARB)(GLenum, GLenum, GLsizei, GLdouble *);
    void  (*glGetnMapfvARB)(GLenum, GLenum, GLsizei, GLfloat *);
    void  (*glGetnMapivARB)(GLenum, GLenum, GLsizei, GLint *);
    void  (*glGetnMinmaxARB)(GLenum, GLboolean, GLenum, GLenum, GLsizei, GLvoid *);
    void  (*glGetnPixelMapfvARB)(GLenum, GLsizei, GLfloat *);
    void  (*glGetnPixelMapuivARB)(GLenum, GLsizei, GLuint *);
    void  (*glGetnPixelMapusvARB)(GLenum, GLsizei, GLushort *);
    void  (*glGetnPolygonStippleARB)(GLsizei, GLubyte *);
    void  (*glGetnSeparableFilterARB)(GLenum, GLenum, GLenum, GLsizei, GLvoid *, GLsizei, GLvoid *, GLvoid *);
    void  (*glGetnTexImageARB)(GLenum, GLint, GLenum, GLenum, GLsizei, GLvoid *);
    void  (*glGetnUniformdvARB)(GLhandleARB, GLint, GLsizei, GLdouble *);
    void  (*glGetnUniformfvARB)(GLhandleARB, GLint, GLsizei, GLfloat *);
    void  (*glGetnUniformivARB)(GLhandleARB, GLint, GLsizei, GLint *);
    void  (*glGetnUniformuivARB)(GLhandleARB, GLint, GLsizei, GLuint *);
    void  (*glReadnPixelsARB)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLsizei, GLvoid *);
    void  (*glDrawArraysInstancedBaseInstance)(GLenum, GLint, GLsizei, GLsizei, GLuint);
    void  (*glDrawElementsInstancedBaseInstance)(GLenum, GLsizei, GLenum, const GLvoid *, GLsizei, GLuint);
    void  (*glDrawElementsInstancedBaseVertexBaseInstance)(GLenum, GLsizei, GLenum, const GLvoid *, GLsizei, GLint, GLuint);
    void  (*glDrawTransformFeedbackInstanced)(GLenum, GLuint, GLsizei);
    void  (*glDrawTransformFeedbackStreamInstanced)(GLenum, GLuint, GLuint, GLsizei);
    void  (*glTexStorage1D)(GLenum, GLsizei, GLenum, GLsizei);
    void  (*glTexStorage2D)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
    void  (*glTexStorage3D)(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei);
    void  (*glTextureStorage1DEXT)(GLuint, GLenum, GLsizei, GLenum, GLsizei);
    void  (*glTextureStorage2DEXT)(GLuint, GLenum, GLsizei, GLenum, GLsizei, GLsizei);
    void  (*glTextureStorage3DEXT)(GLuint, GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei);
    void  (*glPolygonOffsetEXT)(GLfloat, GLfloat);
    void  (*glSampleMaskSGIS)(GLclampf, GLboolean);
    void  (*glSamplePatternSGIS)(GLenum);
    void  (*glColorPointerEXT)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);
    void  (*glEdgeFlagPointerEXT)(GLsizei, GLsizei, const GLboolean *);
    void  (*glIndexPointerEXT)(GLenum, GLsizei, GLsizei, const GLvoid *);
    void  (*glNormalPointerEXT)(GLenum, GLsizei, GLsizei, const GLvoid *);
    void  (*glTexCoordPointerEXT)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);
    void  (*glVertexPointerEXT)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);
    void  (*glPointParameterfEXT)(GLenum, GLfloat);
    void  (*glPointParameterfvEXT)(GLenum, const GLfloat *);
    void  (*glLockArraysEXT)(GLint, GLsizei);
    void  (*glUnlockArraysEXT)(void);
    void  (*glSecondaryColor3bEXT)(GLbyte, GLbyte, GLbyte);
    void  (*glSecondaryColor3bvEXT)(const GLbyte *);
    void  (*glSecondaryColor3dEXT)(GLdouble, GLdouble, GLdouble);
    void  (*glSecondaryColor3dvEXT)(const GLdouble *);
    void  (*glSecondaryColor3fEXT)(GLfloat, GLfloat, GLfloat);
    void  (*glSecondaryColor3fvEXT)(const GLfloat *);
    void  (*glSecondaryColor3iEXT)(GLint, GLint, GLint);
    void  (*glSecondaryColor3ivEXT)(const GLint *);
    void  (*glSecondaryColor3sEXT)(GLshort, GLshort, GLshort);
    void  (*glSecondaryColor3svEXT)(const GLshort *);
    void  (*glSecondaryColor3ubEXT)(GLubyte, GLubyte, GLubyte);
    void  (*glSecondaryColor3ubvEXT)(const GLubyte *);
    void  (*glSecondaryColor3uiEXT)(GLuint, GLuint, GLuint);
    void  (*glSecondaryColor3uivEXT)(const GLuint *);
    void  (*glSecondaryColor3usEXT)(GLushort, GLushort, GLushort);
    void  (*glSecondaryColor3usvEXT)(const GLushort *);
    void  (*glSecondaryColorPointerEXT)(GLint, GLenum, GLsizei, const GLvoid *);
    void  (*glMultiDrawArraysEXT)(GLenum, const GLint *, const GLsizei *, GLsizei);
    void  (*glMultiDrawElementsEXT)(GLenum, const GLsizei *, GLenum, const GLvoid **, GLsizei);
    void  (*glFogCoordPointerEXT)(GLenum, GLsizei, const GLvoid *);
    void  (*glFogCoorddEXT)(GLdouble);
    void  (*glFogCoorddvEXT)(const GLdouble *);
    void  (*glFogCoordfEXT)(GLfloat);
    void  (*glFogCoordfvEXT)(const GLfloat *);
    void  (*glResizeBuffersMESA)(void);
    void  (*glWindowPos2dMESA)(GLdouble, GLdouble);
    void  (*glWindowPos2dvMESA)(const GLdouble *);
    void  (*glWindowPos2fMESA)(GLfloat, GLfloat);
    void  (*glWindowPos2fvMESA)(const GLfloat *);
    void  (*glWindowPos2iMESA)(GLint, GLint);
    void  (*glWindowPos2ivMESA)(const GLint *);
    void  (*glWindowPos2sMESA)(GLshort, GLshort);
    void  (*glWindowPos2svMESA)(const GLshort *);
    void  (*glWindowPos3dMESA)(GLdouble, GLdouble, GLdouble);
    void  (*glWindowPos3dvMESA)(const GLdouble *);
    void  (*glWindowPos3fMESA)(GLfloat, GLfloat, GLfloat);
    void  (*glWindowPos3fvMESA)(const GLfloat *);
    void  (*glWindowPos3iMESA)(GLint, GLint, GLint);
    void  (*glWindowPos3ivMESA)(const GLint *);
    void  (*glWindowPos3sMESA)(GLshort, GLshort, GLshort);
    void  (*glWindowPos3svMESA)(const GLshort *);
    void  (*glWindowPos4dMESA)(GLdouble, GLdouble, GLdouble, GLdouble);
    void  (*glWindowPos4dvMESA)(const GLdouble *);
    void  (*glWindowPos4fMESA)(GLfloat, GLfloat, GLfloat, GLfloat);
    void  (*glWindowPos4fvMESA)(const GLfloat *);
    void  (*glWindowPos4iMESA)(GLint, GLint, GLint, GLint);
    void  (*glWindowPos4ivMESA)(const GLint *);
    void  (*glWindowPos4sMESA)(GLshort, GLshort, GLshort, GLshort);
    void  (*glWindowPos4svMESA)(const GLshort *);
    void  (*glMultiModeDrawArraysIBM)(const GLenum *, const GLint *, const GLsizei *, GLsizei, GLint);
    void  (*glMultiModeDrawElementsIBM)(const GLenum *, const GLsizei *, GLenum, const GLvoid * const *, GLsizei, GLint);
    GLboolean  (*glAreProgramsResidentNV)(GLsizei, const GLuint *, GLboolean *);
    void  (*glBindProgramNV)(GLenum, GLuint);
    void  (*glDeleteProgramsNV)(GLsizei, const GLuint *);
    void  (*glExecuteProgramNV)(GLenum, GLuint, const GLfloat *);
    void  (*glGenProgramsNV)(GLsizei, GLuint *);
    void  (*glGetProgramParameterdvNV)(GLenum, GLuint, GLenum, GLdouble *);
    void  (*glGetProgramParameterfvNV)(GLenum, GLuint, GLenum, GLfloat *);
    void  (*glGetProgramStringNV)(GLuint, GLenum, GLubyte *);
    void  (*glGetProgramivNV)(GLuint, GLenum, GLint *);
    void  (*glGetTrackMatrixivNV)(GLenum, GLuint, GLenum, GLint *);
    void  (*glGetVertexAttribPointervNV)(GLuint, GLenum, GLvoid **);
    void  (*glGetVertexAttribdvNV)(GLuint, GLenum, GLdouble *);
    void  (*glGetVertexAttribfvNV)(GLuint, GLenum, GLfloat *);
    void  (*glGetVertexAttribivNV)(GLuint, GLenum, GLint *);
    GLboolean  (*glIsProgramNV)(GLuint);
    void  (*glLoadProgramNV)(GLenum, GLuint, GLsizei, const GLubyte *);
    void  (*glProgramParameters4dvNV)(GLenum, GLuint, GLsizei, const GLdouble *);
    void  (*glProgramParameters4fvNV)(GLenum, GLuint, GLsizei, const GLfloat *);
    void  (*glRequestResidentProgramsNV)(GLsizei, const GLuint *);
    void  (*glTrackMatrixNV)(GLenum, GLuint, GLenum, GLenum);
    void  (*glVertexAttrib1dNV)(GLuint, GLdouble);
    void  (*glVertexAttrib1dvNV)(GLuint, const GLdouble *);
    void  (*glVertexAttrib1fNV)(GLuint, GLfloat);
    void  (*glVertexAttrib1fvNV)(GLuint, const GLfloat *);
    void  (*glVertexAttrib1sNV)(GLuint, GLshort);
    void  (*glVertexAttrib1svNV)(GLuint, const GLshort *);
    void  (*glVertexAttrib2dNV)(GLuint, GLdouble, GLdouble);
    void  (*glVertexAttrib2dvNV)(GLuint, const GLdouble *);
    void  (*glVertexAttrib2fNV)(GLuint, GLfloat, GLfloat);
    void  (*glVertexAttrib2fvNV)(GLuint, const GLfloat *);
    void  (*glVertexAttrib2sNV)(GLuint, GLshort, GLshort);
    void  (*glVertexAttrib2svNV)(GLuint, const GLshort *);
    void  (*glVertexAttrib3dNV)(GLuint, GLdouble, GLdouble, GLdouble);
    void  (*glVertexAttrib3dvNV)(GLuint, const GLdouble *);
    void  (*glVertexAttrib3fNV)(GLuint, GLfloat, GLfloat, GLfloat);
    void  (*glVertexAttrib3fvNV)(GLuint, const GLfloat *);
    void  (*glVertexAttrib3sNV)(GLuint, GLshort, GLshort, GLshort);
    void  (*glVertexAttrib3svNV)(GLuint, const GLshort *);
    void  (*glVertexAttrib4dNV)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble);
    void  (*glVertexAttrib4dvNV)(GLuint, const GLdouble *);
    void  (*glVertexAttrib4fNV)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
    void  (*glVertexAttrib4fvNV)(GLuint, const GLfloat *);
    void  (*glVertexAttrib4sNV)(GLuint, GLshort, GLshort, GLshort, GLshort);
    void  (*glVertexAttrib4svNV)(GLuint, const GLshort *);
    void  (*glVertexAttrib4ubNV)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte);
    void  (*glVertexAttrib4ubvNV)(GLuint, const GLubyte *);
    void  (*glVertexAttribPointerNV)(GLuint, GLint, GLenum, GLsizei, const GLvoid *);
    void  (*glVertexAttribs1dvNV)(GLuint, GLsizei, const GLdouble *);
    void  (*glVertexAttribs1fvNV)(GLuint, GLsizei, const GLfloat *);
    void  (*glVertexAttribs1svNV)(GLuint, GLsizei, const GLshort *);
    void  (*glVertexAttribs2dvNV)(GLuint, GLsizei, const GLdouble *);
    void  (*glVertexAttribs2fvNV)(GLuint, GLsizei, const GLfloat *);
    void  (*glVertexAttribs2svNV)(GLuint, GLsizei, const GLshort *);
    void  (*glVertexAttribs3dvNV)(GLuint, GLsizei, const GLdouble *);
    void  (*glVertexAttribs3fvNV)(GLuint, GLsizei, const GLfloat *);
    void  (*glVertexAttribs3svNV)(GLuint, GLsizei, const GLshort *);
    void  (*glVertexAttribs4dvNV)(GLuint, GLsizei, const GLdouble *);
    void  (*glVertexAttribs4fvNV)(GLuint, GLsizei, const GLfloat *);
    void  (*glVertexAttribs4svNV)(GLuint, GLsizei, const GLshort *);
    void  (*glVertexAttribs4ubvNV)(GLuint, GLsizei, const GLubyte *);
    void  (*glGetTexBumpParameterfvATI)(GLenum, GLfloat *);
    void  (*glGetTexBumpParameterivATI)(GLenum, GLint *);
    void  (*glTexBumpParameterfvATI)(GLenum, const GLfloat *);
    void  (*glTexBumpParameterivATI)(GLenum, const GLint *);
    void  (*glAlphaFragmentOp1ATI)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint);
    void  (*glAlphaFragmentOp2ATI)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
    void  (*glAlphaFragmentOp3ATI)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
    void  (*glBeginFragmentShaderATI)(void);
    void  (*glBindFragmentShaderATI)(GLuint);
    void  (*glColorFragmentOp1ATI)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
    void  (*glColorFragmentOp2ATI)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
    void  (*glColorFragmentOp3ATI)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
    void  (*glDeleteFragmentShaderATI)(GLuint);
    void  (*glEndFragmentShaderATI)(void);
    GLuint  (*glGenFragmentShadersATI)(GLuint);
    void  (*glPassTexCoordATI)(GLuint, GLuint, GLenum);
    void  (*glSampleMapATI)(GLuint, GLuint, GLenum);
    void  (*glSetFragmentShaderConstantATI)(GLuint, const GLfloat *);
    void  (*glPointParameteriNV)(GLenum, GLint);
    void  (*glPointParameterivNV)(GLenum, const GLint *);
    void  (*glActiveStencilFaceEXT)(GLenum);
    void  (*glBindVertexArrayAPPLE)(GLuint);
    void  (*glDeleteVertexArraysAPPLE)(GLsizei, const GLuint *);
    void  (*glGenVertexArraysAPPLE)(GLsizei, GLuint *);
    GLboolean  (*glIsVertexArrayAPPLE)(GLuint);
    void  (*glGetProgramNamedParameterdvNV)(GLuint, GLsizei, const GLubyte *, GLdouble *);
    void  (*glGetProgramNamedParameterfvNV)(GLuint, GLsizei, const GLubyte *, GLfloat *);
    void  (*glProgramNamedParameter4dNV)(GLuint, GLsizei, const GLubyte *, GLdouble, GLdouble, GLdouble, GLdouble);
    void  (*glProgramNamedParameter4dvNV)(GLuint, GLsizei, const GLubyte *, const GLdouble *);
    void  (*glProgramNamedParameter4fNV) (GLuint, GLsizei, const GLubyte *, GLfloat, GLfloat, GLfloat, GLfloat);
    void  (*glProgramNamedParameter4fvNV)(GLuint, GLsizei, const GLubyte *, const GLfloat *);
    void  (*glPrimitiveRestartIndexNV)(GLuint);
    void  (*glPrimitiveRestartNV)(void);
    void  (*glDepthBoundsEXT)(GLclampd, GLclampd);
    void  (*glFramebufferRenderbufferEXT)(GLenum, GLenum, GLenum, GLuint);
    void  (*glFramebufferTexture1DEXT)(GLenum, GLenum, GLenum, GLuint, GLint);
    void  (*glFramebufferTexture2DEXT)(GLenum, GLenum, GLenum, GLuint, GLint);
    void  (*glFramebufferTexture3DEXT)(GLenum, GLenum, GLenum, GLuint, GLint, GLint);
    void  (*glGenFramebuffersEXT)(GLsizei, GLuint *);
    void  (*glGenRenderbuffersEXT)(GLsizei, GLuint *);
    void  (*glGenerateMipmapEXT)(GLenum);
    void  (*glGetFramebufferAttachmentParameterivEXT)(GLenum, GLenum, GLenum, GLint *);
    void  (*glGetRenderbufferParameterivEXT)(GLenum, GLenum, GLint *);
    GLboolean (*glIsFramebufferEXT)(GLuint);
    GLboolean  (*glIsRenderbufferEXT)(GLuint);
    void  (*glRenderbufferStorageEXT)(GLenum, GLenum, GLsizei, GLsizei);
    void  (*glBlitFramebufferEXT)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
    void  (*glBufferParameteriAPPLE)(GLenum, GLenum, GLint);
    void  (*glFlushMappedBufferRangeAPPLE)(GLenum, GLintptr, GLsizeiptr);
    void  (*glBindFragDataLocationEXT)(GLuint, GLuint, const GLchar *);
    GLint  (*glGetFragDataLocationEXT)(GLuint, const GLchar *);
    void  (*glGetUniformuivEXT)(GLuint, GLint, GLuint *);
    void  (*glGetVertexAttribIivEXT)(GLuint, GLenum, GLint *);
    void  (*glGetVertexAttribIuivEXT)(GLuint, GLenum, GLuint *);
    void  (*glUniform1uiEXT)(GLint, GLuint);
    void  (*glUniform1uivEXT)(GLint, GLsizei, const GLuint *);
    void  (*glUniform2uiEXT)(GLint, GLuint, GLuint);
    void  (*glUniform2uivEXT)(GLint, GLsizei, const GLuint *);
    void  (*glUniform3uiEXT)(GLint, GLuint, GLuint, GLuint);
    void  (*glUniform3uivEXT)(GLint, GLsizei, const GLuint *);
    void  (*glUniform4uiEXT)(GLint, GLuint, GLuint, GLuint, GLuint);
    void  (*glUniform4uivEXT)(GLint, GLsizei, const GLuint *);
    void  (*glVertexAttribI1iEXT)(GLuint, GLint);
    void  (*glVertexAttribI1ivEXT)(GLuint, const GLint *);
    void  (*glVertexAttribI1uiEXT)(GLuint, GLuint);
    void  (*glVertexAttribI1uivEXT)(GLuint, const GLuint *);
    void  (*glVertexAttribI2iEXT)(GLuint, GLint, GLint);
    void  (*glVertexAttribI2ivEXT)(GLuint, const GLint *);
    void  (*glVertexAttribI2uiEXT)(GLuint, GLuint, GLuint);
    void  (*glVertexAttribI2uivEXT)(GLuint, const GLuint *);
    void  (*glVertexAttribI3iEXT)(GLuint, GLint, GLint, GLint);
    void  (*glVertexAttribI3uiEXT)(GLuint, GLuint, GLuint, GLuint);
    void  (*glVertexAttribI3ivEXT)(GLuint, const GLint *);
    void  (*glVertexAttribI3uivEXT)(GLuint, const GLuint *);
    void  (*glVertexAttribI4bvEXT)(GLuint, const GLbyte *);
    void  (*glVertexAttribI4iEXT)(GLuint, GLint, GLint, GLint, GLint);
    void  (*glVertexAttribI4ivEXT)(GLuint, const GLint *);
    void  (*glVertexAttribI4svEXT)(GLuint, const GLshort *);
    void  (*glVertexAttribI4ubvEXT)(GLuint, const GLubyte *);
    void  (*glVertexAttribI4uiEXT)(GLuint, GLuint, GLuint, GLuint, GLuint);
    void  (*glVertexAttribI4uivEXT)(GLuint, const GLuint *);
    void  (*glVertexAttribI4usvEXT)(GLuint, const GLushort *);
    void  (*glVertexAttribIPointerEXT)(GLuint, GLint, GLenum, GLsizei, const GLvoid *);
    void  (*glFramebufferTextureLayerEXT)(GLenum, GLenum, GLuint, GLint, GLint);
    void  (*glColorMaskIndexedEXT)(GLuint, GLboolean, GLboolean, GLboolean, GLboolean);
    void  (*glDisableIndexedEXT)(GLenum, GLuint);
    void  (*glEnableIndexedEXT)(GLenum, GLuint);
    void  (*glGetBooleanIndexedvEXT)(GLenum, GLuint, GLboolean *);
    void  (*glGetIntegerIndexedvEXT)(GLenum, GLuint, GLint *);
    GLboolean  (*glIsEnabledIndexedEXT)(GLenum, GLuint);
    void  (*glClearColorIiEXT)(GLint, GLint, GLint, GLint);
    void  (*glClearColorIuiEXT)(GLuint, GLuint, GLuint, GLuint);
    void  (*glGetTexParameterIuivEXT)(GLenum, GLenum, GLuint *);
    void  (*glGetTexParameterIivEXT)(GLenum, GLenum, GLint *);
    void  (*glTexParameterIivEXT)(GLenum, GLenum, const GLint *);
    void  (*glTexParameterIuivEXT)(GLenum, GLenum, const GLuint *);
    void  (*glBeginConditionalRenderNV)(GLuint, GLenum);
    void  (*glEndConditionalRenderNV)(void);
    void  (*glBeginTransformFeedbackEXT)(GLenum);
    void  (*glBindBufferBaseEXT)(GLenum, GLuint, GLuint);
    void  (*glBindBufferOffsetEXT)(GLenum, GLuint, GLuint, GLintptr);
    void  (*glBindBufferRangeEXT)(GLenum, GLuint, GLuint, GLintptr, GLsizeiptr);
    void  (*glEndTransformFeedbackEXT)(void);
    void  (*glGetTransformFeedbackVaryingEXT)(GLuint, GLuint, GLsizei, GLsizei *, GLsizei *, GLenum *, GLchar *);
    void  (*glTransformFeedbackVaryingsEXT)(GLuint, GLsizei, const char **, GLenum);
    void  (*glProvokingVertexEXT)(GLenum);
    void  (*glGetObjectParameterivAPPLE)(GLenum, GLuint, GLenum, GLint *);
    GLenum  (*glObjectPurgeableAPPLE)(GLenum, GLuint, GLenum);
    GLenum  (*glObjectUnpurgeableAPPLE)(GLenum, GLuint, GLenum);
    void  (*glActiveProgramEXT)(GLuint);
    GLuint  (*glCreateShaderProgramEXT)(GLenum, const GLchar *);
    void  (*glUseShaderProgramEXT)(GLenum, GLuint);
    void  (*glTextureBarrierNV)(void);
    void  (*glStencilFuncSeparateATI)(GLenum, GLenum, GLint, GLuint);
    void  (*glProgramEnvParameters4fvEXT)(GLenum, GLuint, GLsizei, const GLfloat *);
    void  (*glProgramLocalParameters4fvEXT)(GLenum, GLuint, GLsizei, const GLfloat *);
    void  (*glGetQueryObjecti64vEXT)(GLuint, GLenum, GLint64EXT *);
    void  (*glGetQueryObjectui64vEXT)(GLuint, GLenum, GLuint64EXT *);
    void  (*glEGLImageTargetRenderbufferStorageOES)(GLenum, GLvoid *);
#endif
} dispatch_t;

private void
dispatch_init(dispatch_t *dispatch);

#endif
