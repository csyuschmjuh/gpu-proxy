#include "dispatch_private.h"
#include <assert.h>
#include <dlfcn.h>
#include <stdlib.h>

#define DISPATCH_ENTRY_EGL(entryName) dispatch->entryName = dlsym(egl_handle, #entryName)
#define DISPATCH_ENTRY_GL(entryName) find_gl_symbol(gl_handle, dispatch, (FunctionPointerType*) &dispatch->entryName, #entryName, #entryName"ARG", #entryName"EXT")

static void
find_gl_symbol(void *handle,
               dispatch_t *dispatch,
               FunctionPointerType *slot,
               const char *symbol_name,
               const char *symbol_name_alt1,
               const char *symbol_name_alt2)
{
    if (dispatch->eglGetProcAddress) {
        *slot = dispatch->eglGetProcAddress (symbol_name);
        if (!*slot)
            *slot = dispatch->eglGetProcAddress (symbol_name_alt1);
        if (!*slot)
            *slot = dispatch->eglGetProcAddress (symbol_name_alt2);
        return;
    }

    *slot = dlsym (handle, symbol_name);
    if (!*slot)
        *slot = dlsym (handle, symbol_name_alt1);
    if (!*slot)
        *slot = dlsym (handle, symbol_name_alt2);
}

static const char *
dispatch_libgl ()
{
    static const char *default_libgl_name = "libGLESv2.so";
    const char *libgl = getenv ("GPUPROCESS_LIBGLES_PATH");
    return ! libgl ? default_libgl_name : libgl;
}

static const char *
dispatch_libegl ()
{
    static const char *default_libegl_name = "libEGL.so";
    const char *libegl = getenv ("GPUPROCESS_LIBEGL_PATH");
    return ! libegl ? default_libegl_name : libegl;
}

void
dispatch_init(dispatch_t *dispatch)
{
    void *egl_handle = dlopen (dispatch_libegl (), RTLD_LAZY | RTLD_DEEPBIND);
    void *gl_handle = dlopen (dispatch_libgl (), RTLD_LAZY | RTLD_DEEPBIND);
    DISPATCH_ENTRY_EGL (eglGetError);
    DISPATCH_ENTRY_EGL (eglGetDisplay);
    DISPATCH_ENTRY_EGL (eglInitialize);
    DISPATCH_ENTRY_EGL (eglTerminate);
    DISPATCH_ENTRY_EGL (eglQueryString);
    DISPATCH_ENTRY_EGL (eglGetConfigs);
    DISPATCH_ENTRY_EGL (eglChooseConfig);
    DISPATCH_ENTRY_EGL (eglGetConfigAttrib);
    DISPATCH_ENTRY_EGL (eglCreateWindowSurface);
    DISPATCH_ENTRY_EGL (eglCreatePbufferSurface);
    DISPATCH_ENTRY_EGL (eglCreatePixmapSurface);
    DISPATCH_ENTRY_EGL (eglDestroySurface);
    DISPATCH_ENTRY_EGL (eglQuerySurface);
    DISPATCH_ENTRY_EGL (eglBindAPI);
    DISPATCH_ENTRY_EGL (eglQueryAPI);
    DISPATCH_ENTRY_EGL (eglWaitClient);
    DISPATCH_ENTRY_EGL (eglReleaseThread);
    DISPATCH_ENTRY_EGL (eglCreatePbufferFromClientBuffer);
    DISPATCH_ENTRY_EGL (eglSurfaceAttrib);
    DISPATCH_ENTRY_EGL (eglBindTexImage);
    DISPATCH_ENTRY_EGL (eglReleaseTexImage);
    DISPATCH_ENTRY_EGL (eglSwapInterval);
    DISPATCH_ENTRY_EGL (eglCreateContext);
    DISPATCH_ENTRY_EGL (eglDestroyContext);
    DISPATCH_ENTRY_EGL (eglMakeCurrent);
    DISPATCH_ENTRY_EGL (eglGetCurrentContext);
    DISPATCH_ENTRY_EGL (eglGetCurrentSurface);
    DISPATCH_ENTRY_EGL (eglGetCurrentDisplay);
    DISPATCH_ENTRY_EGL (eglQueryContext);
    DISPATCH_ENTRY_EGL (eglWaitGL);
    DISPATCH_ENTRY_EGL (eglWaitNative);
    DISPATCH_ENTRY_EGL (eglSwapBuffers);
    DISPATCH_ENTRY_EGL (eglCopyBuffers);
    DISPATCH_ENTRY_EGL (eglGetProcAddress);

#ifdef EGL_KHR_lock_surface
    DISPATCH_ENTRY_EGL (eglLockSurfaceKHR);
    DISPATCH_ENTRY_EGL (eglUnlockSurfaceKHR);
#endif

#ifdef EGL_KHR_image
    DISPATCH_ENTRY_EGL (eglCreateImageKHR);
    DISPATCH_ENTRY_EGL (eglDestroyImageKHR);
#endif

#ifdef EGL_KHR_reusable_sync
    DISPATCH_ENTRY_EGL (eglCreateSyncKHR);
    DISPATCH_ENTRY_EGL (eglDestroySyncKHR);
    DISPATCH_ENTRY_EGL (eglClientWaitSyncKHR);
    DISPATCH_ENTRY_EGL (eglSignalSyncKHR);
    DISPATCH_ENTRY_EGL (eglGetSyncAttribKHR);
#endif

#ifdef EGL_NV_sync
    DISPATCH_ENTRY_EGL (eglCreateFenceSyncNV);
    DISPATCH_ENTRY_EGL (eglDestroySyncNV);
    DISPATCH_ENTRY_EGL (eglFenceNV);
    DISPATCH_ENTRY_EGL (eglClientWaitSyncNV);
    DISPATCH_ENTRY_EGL (eglSignalSyncNV);
    DISPATCH_ENTRY_EGL (eglGetSyncAttribNV);
#endif

#ifdef EGL_HI_clientpixmap
    DISPATCH_ENTRY_EGL (eglCreatePixmapSurfaceHI);
#endif

#ifdef EGL_MESA_drm_image
    DISPATCH_ENTRY_EGL (eglCreateDRMImageMESA);
    DISPATCH_ENTRY_EGL (eglExportDRMImageMESA);
#endif

#ifdef EGL_POST_SUB_BUFFER_SUPPORTED_NV
    DISPATCH_ENTRY_EGL (eglPostSubBufferNV);
#endif

#ifdef EGL_SEC_image_map
    DISPATCH_ENTRY_EGL (eglMapImageSEC);
    DISPATCH_ENTRY_EGL (eglUnmapImageSEC);
    DISPATCH_ENTRY_EGL (eglGetImageAttribSEC);
#endif

    DISPATCH_ENTRY_GL (glActiveTexture);
    DISPATCH_ENTRY_GL (glAttachShader);
    DISPATCH_ENTRY_GL (glBindAttribLocation);
    DISPATCH_ENTRY_GL (glBindBuffer);
    DISPATCH_ENTRY_GL (glBindFramebuffer);
    DISPATCH_ENTRY_GL (glBindRenderbuffer);
    DISPATCH_ENTRY_GL (glBindTexture);
    DISPATCH_ENTRY_GL (glBlendColor);
    DISPATCH_ENTRY_GL (glBlendEquation);
    DISPATCH_ENTRY_GL (glBlendEquationSeparate);
    DISPATCH_ENTRY_GL (glBlendFunc);
    DISPATCH_ENTRY_GL (glBlendFuncSeparate);
    DISPATCH_ENTRY_GL (glBufferData);
    DISPATCH_ENTRY_GL (glBufferSubData);
    DISPATCH_ENTRY_GL (glCheckFramebufferStatus);
    DISPATCH_ENTRY_GL (glClear);
    DISPATCH_ENTRY_GL (glClearColor);
    DISPATCH_ENTRY_GL (glClearDepthf);
    DISPATCH_ENTRY_GL (glClearStencil);
    DISPATCH_ENTRY_GL (glColorMask);
    DISPATCH_ENTRY_GL (glCompileShader);
    DISPATCH_ENTRY_GL (glCompressedTexImage2D);
    DISPATCH_ENTRY_GL (glCompressedTexSubImage2D);
    DISPATCH_ENTRY_GL (glCopyTexImage2D);
    DISPATCH_ENTRY_GL (glCopyTexSubImage2D);
    DISPATCH_ENTRY_GL (glCreateProgram);
    DISPATCH_ENTRY_GL (glCreateShader);
    DISPATCH_ENTRY_GL (glCullFace);
    DISPATCH_ENTRY_GL (glDeleteBuffers);
    DISPATCH_ENTRY_GL (glDeleteFramebuffers);
    DISPATCH_ENTRY_GL (glDeleteProgram);
    DISPATCH_ENTRY_GL (glDeleteRenderbuffers);
    DISPATCH_ENTRY_GL (glDeleteShader);
    DISPATCH_ENTRY_GL (glDeleteTextures);
    DISPATCH_ENTRY_GL (glDepthFunc);
    DISPATCH_ENTRY_GL (glDepthMask);
    DISPATCH_ENTRY_GL (glDepthRangef);
    DISPATCH_ENTRY_GL (glDetachShader);
    DISPATCH_ENTRY_GL (glDisable);
    DISPATCH_ENTRY_GL (glDisableVertexAttribArray);
    DISPATCH_ENTRY_GL (glDrawArrays);
    DISPATCH_ENTRY_GL (glDrawElements);
    DISPATCH_ENTRY_GL (glEnable);
    DISPATCH_ENTRY_GL (glEnableVertexAttribArray);
    DISPATCH_ENTRY_GL (glFinish);
    DISPATCH_ENTRY_GL (glFlush);
    DISPATCH_ENTRY_GL (glFramebufferRenderbuffer);
    DISPATCH_ENTRY_GL (glFramebufferTexture2D);
    DISPATCH_ENTRY_GL (glFrontFace);
    DISPATCH_ENTRY_GL (glGenBuffers);
    DISPATCH_ENTRY_GL (glGenerateMipmap);
    DISPATCH_ENTRY_GL (glGenFramebuffers);
    DISPATCH_ENTRY_GL (glGenRenderbuffers);
    DISPATCH_ENTRY_GL (glGenTextures);
    DISPATCH_ENTRY_GL (glGetActiveAttrib);
    DISPATCH_ENTRY_GL (glGetActiveUniform);
    DISPATCH_ENTRY_GL (glGetAttachedShaders);
    DISPATCH_ENTRY_GL (glGetAttribLocation);
    DISPATCH_ENTRY_GL (glGetBooleanv);
    DISPATCH_ENTRY_GL (glGetBufferParameteriv);
    DISPATCH_ENTRY_GL (glGetError);
    DISPATCH_ENTRY_GL (glGetFloatv);
    DISPATCH_ENTRY_GL (glGetFramebufferAttachmentParameteriv);
    DISPATCH_ENTRY_GL (glGetIntegerv);
    DISPATCH_ENTRY_GL (glGetProgramiv);
    DISPATCH_ENTRY_GL (glGetProgramInfoLog);
    DISPATCH_ENTRY_GL (glGetRenderbufferParameteriv);
    DISPATCH_ENTRY_GL (glGetShaderiv);
    DISPATCH_ENTRY_GL (glGetShaderInfoLog);
    DISPATCH_ENTRY_GL (glGetShaderPrecisionFormat);
    DISPATCH_ENTRY_GL (glGetShaderSource);
    DISPATCH_ENTRY_GL (glGetString);
    DISPATCH_ENTRY_GL (glGetTexParameterfv);
    DISPATCH_ENTRY_GL (glGetTexParameteriv);
    DISPATCH_ENTRY_GL (glGetUniformfv);
    DISPATCH_ENTRY_GL (glGetUniformiv);
    DISPATCH_ENTRY_GL (glGetUniformLocation);
    DISPATCH_ENTRY_GL (glGetVertexAttribfv);
    DISPATCH_ENTRY_GL (glGetVertexAttribiv);
    DISPATCH_ENTRY_GL (glGetVertexAttribPointerv);
    DISPATCH_ENTRY_GL (glHint);
    DISPATCH_ENTRY_GL (glIsBuffer);
    DISPATCH_ENTRY_GL (glIsEnabled);
    DISPATCH_ENTRY_GL (glIsFramebuffer);
    DISPATCH_ENTRY_GL (glIsProgram);
    DISPATCH_ENTRY_GL (glIsRenderbuffer);
    DISPATCH_ENTRY_GL (glIsShader);
    DISPATCH_ENTRY_GL (glIsTexture);
    DISPATCH_ENTRY_GL (glLineWidth);
    DISPATCH_ENTRY_GL (glLinkProgram);
    DISPATCH_ENTRY_GL (glPixelStorei);
    DISPATCH_ENTRY_GL (glPolygonOffset);
    DISPATCH_ENTRY_GL (glReadPixels);
    DISPATCH_ENTRY_GL (glReleaseShaderCompiler);
    DISPATCH_ENTRY_GL (glRenderbufferStorage);
    DISPATCH_ENTRY_GL (glSampleCoverage);
    DISPATCH_ENTRY_GL (glScissor);
    DISPATCH_ENTRY_GL (glShaderBinary);
    DISPATCH_ENTRY_GL (glShaderSource);
    DISPATCH_ENTRY_GL (glStencilFunc);
    DISPATCH_ENTRY_GL (glStencilFuncSeparate);
    DISPATCH_ENTRY_GL (glStencilMask);
    DISPATCH_ENTRY_GL (glStencilMaskSeparate);
    DISPATCH_ENTRY_GL (glStencilOp);
    DISPATCH_ENTRY_GL (glStencilOpSeparate);
    DISPATCH_ENTRY_GL (glTexImage2D);
    DISPATCH_ENTRY_GL (glTexParameterf);
    DISPATCH_ENTRY_GL (glTexParameterfv);
    DISPATCH_ENTRY_GL (glTexParameteri);
    DISPATCH_ENTRY_GL (glTexParameteriv);
    DISPATCH_ENTRY_GL (glTexSubImage2D);
    DISPATCH_ENTRY_GL (glUniform1f);
    DISPATCH_ENTRY_GL (glUniform1fv);
    DISPATCH_ENTRY_GL (glUniform1i);
    DISPATCH_ENTRY_GL (glUniform1iv);
    DISPATCH_ENTRY_GL (glUniform2f);
    DISPATCH_ENTRY_GL (glUniform2fv);
    DISPATCH_ENTRY_GL (glUniform2i);
    DISPATCH_ENTRY_GL (glUniform2iv);
    DISPATCH_ENTRY_GL (glUniform3f);
    DISPATCH_ENTRY_GL (glUniform3fv);
    DISPATCH_ENTRY_GL (glUniform3i);
    DISPATCH_ENTRY_GL (glUniform3iv);
    DISPATCH_ENTRY_GL (glUniform4f);
    DISPATCH_ENTRY_GL (glUniform4fv);
    DISPATCH_ENTRY_GL (glUniform4i);
    DISPATCH_ENTRY_GL (glUniform4iv);
    DISPATCH_ENTRY_GL (glUniformMatrix2fv);
    DISPATCH_ENTRY_GL (glUniformMatrix3fv);
    DISPATCH_ENTRY_GL (glUniformMatrix4fv);
    DISPATCH_ENTRY_GL (glUseProgram);
    DISPATCH_ENTRY_GL (glValidateProgram);
    DISPATCH_ENTRY_GL (glVertexAttrib1f);
    DISPATCH_ENTRY_GL (glVertexAttrib1fv);
    DISPATCH_ENTRY_GL (glVertexAttrib2f);
    DISPATCH_ENTRY_GL (glVertexAttrib2fv);
    DISPATCH_ENTRY_GL (glVertexAttrib3f);
    DISPATCH_ENTRY_GL (glVertexAttrib3fv);
    DISPATCH_ENTRY_GL (glVertexAttrib4f);
    DISPATCH_ENTRY_GL (glVertexAttrib4fv);
    DISPATCH_ENTRY_GL (glVertexAttribPointer);
    DISPATCH_ENTRY_GL (glViewport);

#ifdef GL_OES_EGL_image
    DISPATCH_ENTRY_GL (glEGLImageTargetTexture2DOES);
    DISPATCH_ENTRY_GL (glEGLImageTargetRenderbufferStorageOES);
#endif

#ifdef GL_OES_get_program_binary
    DISPATCH_ENTRY_GL (glGetProgramBinaryOES);
    DISPATCH_ENTRY_GL (glProgramBinaryOES);
#endif

#ifdef GL_OES_mapbuffer
    DISPATCH_ENTRY_GL (glMapBufferOES);
    DISPATCH_ENTRY_GL (glUnmapBufferOES);
    DISPATCH_ENTRY_GL (glGetBufferPointervOES);
#endif

#ifdef GL_OES_texture_3D
    DISPATCH_ENTRY_GL (glTexImage3DOES);
    DISPATCH_ENTRY_GL (glTexSubImage3DOES);
    DISPATCH_ENTRY_GL (glCopyTexSubImage3DOES);
    DISPATCH_ENTRY_GL (glCompressedTexImage3DOES);
    DISPATCH_ENTRY_GL (glCompressedTexSubImage3DOES);
    DISPATCH_ENTRY_GL (glFramebufferTexture3DOES);
#endif

#ifdef GL_OES_vertex_array_object
    DISPATCH_ENTRY_GL (glBindVertexArrayOES);
    DISPATCH_ENTRY_GL (glDeleteVertexArraysOES);
    DISPATCH_ENTRY_GL (glGenVertexArraysOES);
    DISPATCH_ENTRY_GL (glIsVertexArrayOES);
#endif

#ifdef GL_AMD_performance_monitor
    DISPATCH_ENTRY_GL (glGetPerfMonitorGroupsAMD);
    DISPATCH_ENTRY_GL (glGetPerfMonitorCountersAMD);
    DISPATCH_ENTRY_GL (glGetPerfMonitorGroupStringAMD);
    DISPATCH_ENTRY_GL (glGetPerfMonitorCounterStringAMD);
    DISPATCH_ENTRY_GL (glGetPerfMonitorCounterInfoAMD);
    DISPATCH_ENTRY_GL (glGenPerfMonitorsAMD);
    DISPATCH_ENTRY_GL (glDeletePerfMonitorsAMD);
    DISPATCH_ENTRY_GL (glSelectPerfMonitorCountersAMD);
    DISPATCH_ENTRY_GL (glBeginPerfMonitorAMD);
    DISPATCH_ENTRY_GL (glEndPerfMonitorAMD);
    DISPATCH_ENTRY_GL (glGetPerfMonitorCounterDataAMD);
#endif

#ifdef GL_ANGLE_framebuffer_blit
    DISPATCH_ENTRY_GL (glBlitFramebufferANGLE);
#endif

#ifdef GL_ANGLE_framebuffer_multisample
    DISPATCH_ENTRY_GL (glRenderbufferStorageMultisampleANGLE);
#endif

#ifdef GL_APPLE_framebuffer_multisample
    DISPATCH_ENTRY_GL (glRenderbufferStorageMultisampleAPPLE);
    DISPATCH_ENTRY_GL (glResolveMultisampleFramebufferAPPLE);
#endif

#ifdef GL_IMG_multisampled_render_to_texture
    DISPATCH_ENTRY_GL (glRenderbufferStorageMultisampleIMG);
    DISPATCH_ENTRY_GL (glFramebufferTexture2DMultisampleIMG);
#endif

#ifdef GL_EXT_discard_framebuffer
    DISPATCH_ENTRY_GL (glDiscardFramebufferEXT);
#endif

#ifdef GL_EXT_multi_draw_arrays
    DISPATCH_ENTRY_GL (glMultiDrawArraysEXT);
    DISPATCH_ENTRY_GL (glMultiDrawElementsEXT);
#endif

#ifdef GL_EXT_multisampled_render_to_texture
    DISPATCH_ENTRY_GL (glRenderbufferStorageMultisampleEXT);
    DISPATCH_ENTRY_GL (glFramebufferTexture2DMultisampleEXT);
#endif


#ifdef GL_NV_fence
    DISPATCH_ENTRY_GL (glDeleteFencesNV);
    DISPATCH_ENTRY_GL (glGenFencesNV);
    DISPATCH_ENTRY_GL (glIsFenceNV);
    DISPATCH_ENTRY_GL (glTestFenceNV);
    DISPATCH_ENTRY_GL (glGetFenceivNV);
    DISPATCH_ENTRY_GL (glFinishFenceNV);
    DISPATCH_ENTRY_GL (glSetFenceNV);
#endif

#ifdef GL_NV_coverage_sample
    DISPATCH_ENTRY_GL (glCoverageMaskNV);
    DISPATCH_ENTRY_GL (glCoverageOperationNV);
#endif

#ifdef GL_QCOM_driver_control
    DISPATCH_ENTRY_GL (glGetDriverControlsQCOM);
    DISPATCH_ENTRY_GL (glGetDriverControlStringQCOM);
    DISPATCH_ENTRY_GL (glEnableDriverControlQCOM);
    DISPATCH_ENTRY_GL (glDisableDriverControlQCOM);
#endif

#ifdef GL_QCOM_extended_get
    DISPATCH_ENTRY_GL (glExtGetTexturesQCOM);
    DISPATCH_ENTRY_GL (glExtGetBuffersQCOM);
    DISPATCH_ENTRY_GL (glExtGetRenderbuffersQCOM);
    DISPATCH_ENTRY_GL (glExtGetFramebuffersQCOM);
    DISPATCH_ENTRY_GL (glExtGetTexLevelParameterivQCOM);
    DISPATCH_ENTRY_GL (glExtTexObjectStateOverrideiQCOM);
    DISPATCH_ENTRY_GL (glExtGetTexSubImageQCOM);
    DISPATCH_ENTRY_GL (glExtGetBufferPointervQCOM);
#endif

#ifdef GL_QCOM_extended_get2
    DISPATCH_ENTRY_GL (glExtGetShadersQCOM);
    DISPATCH_ENTRY_GL (glExtGetProgramsQCOM);
    DISPATCH_ENTRY_GL (glExtIsProgramBinaryQCOM);
    DISPATCH_ENTRY_GL (glExtGetProgramBinarySourceQCOM);
#endif

#ifdef GL_QCOM_tiled_rendering
    DISPATCH_ENTRY_GL (glStartTilingQCOM);
    DISPATCH_ENTRY_GL (glEndTilingQCOM);
#endif
}
