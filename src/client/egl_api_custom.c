/* Implemented egl and eglext.h functions.
 * This is the egl functions entry point in the client thread.
 *
 * The server thread is initialized and be ready during eglGetDisplay()
 * and is terminated during eglTerminate() when the current context is
 * None. If a client calls egl functions before eglGetDisplay(), an error
 * is returned.
 * Variables used in the code:
 * (1) active_context - a pointer to the current state that is in
 * global server_states.  If it is NULL, the current client is not in
 * any valid egl context.
 * (2) command_buffer - a pointer to the command buffer that is shared
 * between client thread and server thread.  The client posts command to
 * command buffer, while the server thread reads from the command from
 * the command buffer
 */

#include "config.h"


#include "command_autogen.h"
#include "gles2_api_private.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <dlfcn.h>

static bool
_has_extension (const char* extension_name)
{
    const char* extensions =
        (const char*) server_dispatch_table_get_base ()->glGetString (NULL, GL_EXTENSIONS);

    size_t extension_name_length = strlen (extension_name);
    char* found = strstr (extensions, extension_name);
    while (found) {
        if (found[extension_name_length] || found[extension_name_length] == '\0')
            return true;
        found = strstr (found + extension_name_length, extension_name);
    }
    return false;
}

#define RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(symbol_name) \
if (!strcmp (#symbol_name, procname)) \
    return dlsym (NULL, "__hidden_gpuproxy_"#symbol_name);

EGLAPI __eglMustCastToProperFunctionPointerType EGLAPIENTRY
eglGetProcAddress (const char *procname)
{
    INSTRUMENT();

    if (! on_client_thread ()) {
        return server_dispatch_table_get_base ()->eglGetProcAddress (NULL, procname);
    }

    if (_has_extension ("GL_OES_EGL_image")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glEGLImageTargetTexture2DOES);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glEGLImageTargetRenderbufferStorageOES);
    }

    if (_has_extension ("GL_OES_get_program_binary")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glGetProgramBinaryOES);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glProgramBinaryOES);
    }
    
    if (_has_extension ("GL_OES_mapbuffer")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glMapBufferOES);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glUnmapBufferOES);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glGetBufferPointervOES);
    }

    if (_has_extension ("GL_OES_texture_3D")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glTexImage3DOES);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glTexSubImage3DOES);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glCopyTexSubImage3DOES);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glCompressedTexImage3DOES);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glCompressedTexSubImage3DOES);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glFramebufferTexture3DOES);
    }

    if (_has_extension ("GL_OES_vertex_array_object")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glBindVertexArrayOES);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glDeleteVertexArraysOES);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glGenVertexArraysOES);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glIsVertexArrayOES);
    }

    if (_has_extension ("GL_AMD_performance_monitor")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(glGetPerfMonitorGroupsAMD);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(glGetPerfMonitorCountersAMD);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(glGetPerfMonitorGroupStringAMD);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(glGetPerfMonitorCounterStringAMD);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(glGetPerfMonitorCounterInfoAMD);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(glGenPerfMonitorsAMD);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(glDeletePerfMonitorsAMD);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(glSelectPerfMonitorCountersAMD);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(glBeginPerfMonitorAMD);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(glEndPerfMonitorAMD);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(glGetPerfMonitorCounterDataAMD);
    }

    if (_has_extension ("GL_ANGLE_framebuffer_blit")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(glBlitFramebufferANGLE);
    }

    if (_has_extension ("GL_ANGLE_framebuffer_multisample")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(glBlitFramebufferANGLE);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(glRenderbufferStorageMultisampleANGLE);
    }

    if (_has_extension ("GL_APPLE_framebuffer_multisample")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(glRenderbufferStorageMultisampleAPPLE);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES(glResolveMultisampleFramebufferAPPLE);
    }

    if (_has_extension ("GL_IMG_multisampled_render_to_texture")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glRenderbufferStorageMultisampleIMG);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glFramebufferTexture2DMultisampleIMG);
    }

    if (_has_extension ("GL_EXT_discard_framebuffer")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glDiscardFramebufferEXT);
    }

    if (_has_extension ("GL_EXT_multi_draw_arrays")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glMultiDrawArraysEXT);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glMultiDrawElementsEXT);
    }

    if (_has_extension ("GL_EXT_multisampled_render_to_texture")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glRenderbufferStorageMultisampleEXT);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glFramebufferTexture2DMultisampleEXT);
    }

    if (_has_extension ("GL_NV_fence")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glDeleteFencesNV);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glGenFencesNV);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glIsFenceNV);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glTestFenceNV);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glGetFenceivNV);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glFinishFenceNV);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glSetFenceNV);
    }

    if (_has_extension ("GL_NV_coverage_sample")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glCoverageMaskNV);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glCoverageOperationNV);
    }

    if (_has_extension ("GL_QCOM_driver_control")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glGetDriverControlsQCOM);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glGetDriverControlStringQCOM);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glEnableDriverControlQCOM);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glDisableDriverControlQCOM);
    }

    if (_has_extension ("GL_QCOM_extended_get")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glExtGetTexturesQCOM);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glExtGetBuffersQCOM);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glExtGetRenderbuffersQCOM);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glExtGetFramebuffersQCOM);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glExtGetTexLevelParameterivQCOM);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glExtTexObjectStateOverrideiQCOM);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glExtGetTexSubImageQCOM);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glExtGetBufferPointervQCOM);
    }

    if (_has_extension ("GL_QCOM_extended_get2")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glExtGetShadersQCOM);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glExtGetProgramsQCOM);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glExtIsProgramBinaryQCOM);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glExtGetProgramBinarySourceQCOM);
    }

    if (_has_extension ("GL_QCOM_tiled_rendering")) {
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glStartTilingQCOM);
         RETURN_HIDDEN_SYMBOL_IF_NAME_MATCHES (glEndTilingQCOM);
    }

    if (_is_error_state ())
        return NULL;

    return dlsym (NULL, procname);
}
