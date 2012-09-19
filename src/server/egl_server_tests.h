#ifndef GPUPROCESS_EGL_SEVER_TESTS_H
#define GPUPROCESS_EGL_SEVER_TESTS_H

#include "egl_server_private.h"

#ifdef __cplusplus
extern "C" {
#endif

exposed_to_tests EGLint
_egl_get_error (void);

exposed_to_tests EGLDisplay
_egl_get_display (EGLNativeDisplayType display_id);

exposed_to_tests EGLBoolean
_egl_initialize (EGLDisplay display,
                 EGLint *major,
                 EGLint *minor);

exposed_to_tests EGLBoolean
_egl_terminate (EGLDisplay display);

exposed_to_tests EGLBoolean
_egl_get_configs (EGLDisplay display,
                  EGLConfig *configs, 
                  EGLint config_size,
                  EGLint *num_config);

exposed_to_tests EGLBoolean
_egl_choose_config (EGLDisplay display,
                    const EGLint *attrib_list,
                    EGLConfig *configs,
                    EGLint config_size,
                    EGLint *num_config);

exposed_to_tests EGLBoolean
_egl_get_config_attrib (EGLDisplay display,
                        EGLConfig config,
                        EGLint attribute,
                        EGLint *value);

exposed_to_tests EGLSurface
_egl_create_window_surface (EGLDisplay display,
                            EGLConfig config,
                            EGLNativeWindowType win,
                            const EGLint *attrib_list);


exposed_to_tests EGLSurface
_egl_create_pbuffer_surface (EGLDisplay display,
                             EGLConfig config,
                             const EGLint *attrib_list);

exposed_to_tests EGLSurface
_egl_create_pixmap_surface (EGLDisplay display,
                            EGLConfig config,
                            EGLNativePixmapType pixmap, 
                            const EGLint *attrib_list);

exposed_to_tests EGLBoolean
_egl_destroy_surface (EGLDisplay display,
                      EGLSurface surface);

exposed_to_tests EGLBoolean
_egl_query_surface (EGLDisplay display,
                    EGLSurface surface,
                    EGLint attribute,
                    EGLint *value);

exposed_to_tests EGLBoolean
_egl_bind_api (EGLenum api);

exposed_to_tests EGLContext
_egl_create_context (EGLDisplay display,
                     EGLConfig config,
                     EGLContext share_context,
                     const EGLint *attrib_list);

exposed_to_tests EGLBoolean
_egl_destroy_context (EGLDisplay dpy,
                      EGLContext ctx);

exposed_to_tests EGLContext
_egl_get_current_context (void);

exposed_to_tests EGLDisplay
_egl_get_current_display (void);

exposed_to_tests EGLSurface
_egl_get_current_surface (EGLint readdraw);

exposed_to_tests EGLBoolean
_egl_query_context (EGLDisplay display,
                    EGLContext ctx,
                    EGLint attribute,
                    EGLint *value);

exposed_to_tests EGLBoolean 
_egl_make_current (EGLDisplay display,
                   EGLSurface draw,
                   EGLSurface read,
                   EGLContext ctx);

#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_EGL_SERVER_TESTS_H */
