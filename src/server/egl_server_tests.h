#ifndef GPUPROCESS_EGL_SEVER_TESTS_H
#define GPUPROCESS_EGL_SEVER_TESTS_H

#include "egl_server_private.h"
#include "command_buffer_server.h"

#ifdef __cplusplus
extern "C" {
#endif

exposed_to_tests void
caching_server_init (caching_server_t *server, buffer_t *buffer);

exposed_to_tests caching_server_t *
caching_server_new (buffer_t *buffer);

exposed_to_tests EGLint
_egl_get_error (command_buffer_server_t *server);

exposed_to_tests EGLDisplay
_egl_get_display (command_buffer_server_t *server,
                  EGLNativeDisplayType display_id);

exposed_to_tests EGLBoolean
_egl_initialize (command_buffer_server_t *server,
                 EGLDisplay display,
                 EGLint *major,
                 EGLint *minor);

exposed_to_tests EGLBoolean
_egl_terminate (command_buffer_server_t *server,
                EGLDisplay display);

exposed_to_tests EGLBoolean
_egl_get_configs (command_buffer_server_t *server,
                  EGLDisplay display,
                  EGLConfig *configs, 
                  EGLint config_size,
                  EGLint *num_config);

exposed_to_tests EGLBoolean
_egl_choose_config (command_buffer_server_t *server,
                    EGLDisplay display,
                    const EGLint *attrib_list,
                    EGLConfig *configs,
                    EGLint config_size,
                    EGLint *num_config);

exposed_to_tests EGLBoolean
_egl_get_config_attrib (command_buffer_server_t *server,
                        EGLDisplay display,
                        EGLConfig config,
                        EGLint attribute,
                        EGLint *value);

exposed_to_tests EGLSurface
_egl_create_window_surface (command_buffer_server_t *server,
                            EGLDisplay display,
                            EGLConfig config,
                            EGLNativeWindowType win,
                            const EGLint *attrib_list);


exposed_to_tests EGLSurface
_egl_create_pbuffer_surface (command_buffer_server_t *server,
                             EGLDisplay display,
                             EGLConfig config,
                             const EGLint *attrib_list);

exposed_to_tests EGLSurface
_egl_create_pixmap_surface (command_buffer_server_t *server,
                            EGLDisplay display,
                            EGLConfig config,
                            EGLNativePixmapType pixmap, 
                            const EGLint *attrib_list);

exposed_to_tests EGLBoolean
_egl_destroy_surface (command_buffer_server_t *server,
                      EGLDisplay display,
                      EGLSurface surface);

exposed_to_tests EGLBoolean
_egl_query_surface (command_buffer_server_t *server,
                    EGLDisplay display,
                    EGLSurface surface,
                    EGLint attribute,
                    EGLint *value);

exposed_to_tests EGLBoolean
_egl_bind_api (command_buffer_server_t *server,
               EGLenum api);

exposed_to_tests EGLContext
_egl_create_context (command_buffer_server_t *server,
                     EGLDisplay display,
                     EGLConfig config,
                     EGLContext share_context,
                     const EGLint *attrib_list);

exposed_to_tests EGLBoolean
_egl_destroy_context (command_buffer_server_t *server,
                      EGLDisplay dpy,
                      EGLContext ctx);

exposed_to_tests EGLContext
_egl_get_current_context (command_buffer_server_t *server);

exposed_to_tests EGLDisplay
_egl_get_current_display (command_buffer_server_t *server);

exposed_to_tests EGLSurface
_egl_get_current_surface (command_buffer_server_t *server,
                          EGLint readdraw);

exposed_to_tests EGLBoolean
_egl_query_context (command_buffer_server_t *server,
                    EGLDisplay display,
                    EGLContext ctx,
                    EGLint attribute,
                    EGLint *value);

exposed_to_tests EGLBoolean 
_egl_make_current (command_buffer_server_t *server,
                   EGLDisplay display,
                   EGLSurface draw,
                   EGLSurface read,
                   EGLContext ctx);

#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_EGL_SERVER_TESTS_H */
