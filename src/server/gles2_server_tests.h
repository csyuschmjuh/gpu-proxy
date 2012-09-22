#ifndef GPUPROCESS_GLES_SEVER_TESTS_H
#define GPUPROCESS_GLES_SEVER_TESTS_H
#include "gles2_states.h"
#include "server.h"
#include "compiler_private.h"

#ifdef __cplusplus
extern "C" {
#endif

exposed_to_tests void _gl_set_error(server_t *server, GLenum error);
exposed_to_tests bool _gl_is_valid_func (server_t *server, void *func);
exposed_to_tests bool _gl_is_valid_context (server_t *server);
exposed_to_tests void _gl_active_texture (server_t *server, GLenum texture);
exposed_to_tests void _gl_attach_shader (server_t *server, GLuint program, GLuint shader);
exposed_to_tests void _gl_bind_attrib_location (server_t *server, GLuint program, GLuint index, const GLchar *name);
exposed_to_tests void _gl_bind_buffer (server_t *server, GLenum target, GLuint buffer);
exposed_to_tests void _gl_bind_framebuffer (server_t *server, GLenum target, GLuint framebuffer);
exposed_to_tests void _gl_bind_renderbuffer (server_t *server, GLenum target, GLuint renderbuffer);
exposed_to_tests void _gl_bind_texture (server_t *server, GLenum target, GLuint texture);
exposed_to_tests void _gl_blend_color (server_t *server, GLclampf red, GLclampf green,GLclampf blue, GLclampf alpha);
exposed_to_tests void _gl_blend_equation (server_t *server, GLenum mode);
exposed_to_tests void _gl_blend_equation_separate (server_t *server, GLenum modeRGB, GLenum modeAlpha);
exposed_to_tests void _gl_blend_func (server_t *server, GLenum sfactor, GLenum dfactor);
exposed_to_tests void _gl_blend_func_separate (server_t *server, GLenum srcRGB, GLenum dstRGB,GLenum srcAlpha, GLenum dstAlpha);
exposed_to_tests void _gl_buffer_data (server_t *server, GLenum target, GLsizeiptr size,const GLvoid *data, GLenum usage);
exposed_to_tests void _gl_buffer_sub_data (server_t *server, GLenum target, GLintptr offset,GLsizeiptr size, const GLvoid *data);
exposed_to_tests GLenum _gl_check_framebuffer_status (server_t *server, GLenum target);
exposed_to_tests void _gl_clear (server_t *server, GLbitfield mask);
exposed_to_tests void _gl_clear_color (server_t *server, GLclampf red, GLclampf green,GLclampf blue, GLclampf alpha);
exposed_to_tests void _gl_clear_depthf (server_t *server, GLclampf depth);
exposed_to_tests void _gl_clear_stencil (server_t *server, GLint s);
exposed_to_tests void _gl_color_mask (server_t *server, GLboolean red, GLboolean green,GLboolean blue, GLboolean alpha);
exposed_to_tests void _gl_compressed_tex_image_2d (server_t *server, GLenum target, GLint level,
                                         GLenum internalformat,
                                         GLsizei width, GLsizei height,
                                         GLint border, GLsizei imageSize,
                                         const GLvoid *data);
exposed_to_tests void _gl_compressed_tex_sub_image_2d (server_t *server, GLenum target, GLint level,
                                             GLint xoffset, GLint yoffset,
                                             GLsizei width, GLsizei height,
                                             GLenum format,
                                             GLsizei imageSize,
                                             const GLvoid *data);
exposed_to_tests void _gl_copy_tex_image_2d (server_t *server, GLenum target, GLint level,
                                   GLenum internalformat,
                                   GLint x, GLint y,
                                   GLsizei width, GLsizei height,
                                   GLint border);
exposed_to_tests void _gl_copy_tex_sub_image_2d (server_t *server, GLenum target, GLint level,
                                       GLint xoffset, GLint yoffset,
                                       GLint x, GLint y,
                                       GLsizei width, GLsizei height);
exposed_to_tests GLuint _gl_create_program  (server_t *server);
exposed_to_tests GLuint _gl_create_shader (server_t *server, GLenum shaderType);
exposed_to_tests void _gl_cull_face (server_t *server, GLenum mode);
exposed_to_tests void _gl_delete_buffers (server_t *server, GLsizei n, const GLuint *buffers);
exposed_to_tests void _gl_delete_framebuffers (server_t *server, GLsizei n, const GLuint *framebuffers);
exposed_to_tests void _gl_delete_program (server_t *server, GLuint program);
exposed_to_tests void _gl_delete_renderbuffers (server_t *server, GLsizei n, const GLuint *renderbuffers);
exposed_to_tests void _gl_delete_shader (server_t *server, GLuint shader);
exposed_to_tests void _gl_delete_textures (server_t *server, GLsizei n, const GLuint *textures);
exposed_to_tests void _gl_depth_func (server_t *server, GLenum func);
exposed_to_tests void _gl_depth_mask (server_t *server, GLboolean flag);
exposed_to_tests void _gl_depth_rangef (server_t *server, GLclampf nearVal, GLclampf farVal);
exposed_to_tests void _gl_detach_shader (server_t *server, GLuint program, GLuint shader);
exposed_to_tests void _gl_set_cap (server_t *server, GLenum cap, GLboolean enable);
exposed_to_tests void _gl_disable (server_t *server, GLenum cap);
exposed_to_tests void _gl_enable (server_t *server, GLenum cap);
exposed_to_tests bool _gl_index_is_too_large (server_t *server, gles2_state_t *state, GLuint index);
exposed_to_tests void _gl_set_vertex_attrib_array (server_t *server, GLuint index, gles2_state_t *state,GLboolean enable);
exposed_to_tests void _gl_disable_vertex_attrib_array (server_t *server, GLuint index);
exposed_to_tests void _gl_enable_vertex_attrib_array (server_t *server, GLuint index);
exposed_to_tests char *_gl_create_data_array (server_t *server, vertex_attrib_t *attrib, int count);
exposed_to_tests void _gl_draw_arrays (server_t *server, GLenum mode, GLint first, GLsizei count);
exposed_to_tests char *_gl_create_indices_array (server_t *server, GLenum mode, GLenum type, int count,char *indices);
exposed_to_tests void _gl_draw_elements (server_t *server, GLenum mode, GLsizei count, GLenum type,const GLvoid *indices);
exposed_to_tests void _gl_finish (server_t *server);
exposed_to_tests void _gl_flush (server_t *server);
exposed_to_tests void _gl_framebuffer_renderbuffer (server_t *server, GLenum target, GLenum attachment,
                                          GLenum renderbuffertarget,
                                          GLenum renderbuffer);
exposed_to_tests void _gl_framebuffer_texture_2d (server_t *server, GLenum target, GLenum attachment,
                                        GLenum textarget, GLuint texture,
                                        GLint level);
exposed_to_tests void _gl_front_face (server_t *server, GLenum mode);
exposed_to_tests void _gl_gen_buffers (server_t *server, GLsizei n, GLuint *buffers);
exposed_to_tests void _gl_gen_framebuffers (server_t *server, GLsizei n, GLuint *framebuffers);
exposed_to_tests void _gl_gen_renderbuffers (server_t *server, GLsizei n, GLuint *renderbuffers);
exposed_to_tests void _gl_gen_textures (server_t *server, GLsizei n, GLuint *textures);
exposed_to_tests void _gl_generate_mipmap (server_t *server, GLenum target);
exposed_to_tests void _gl_get_booleanv (server_t *server, GLenum pname, GLboolean *params);
exposed_to_tests void _gl_get_floatv (server_t *server, GLenum pname, GLfloat *params);
exposed_to_tests void _gl_get_integerv (server_t *server, GLenum pname, GLint *params);
exposed_to_tests void _gl_get_active_attrib (server_t *server, GLuint program, GLuint index,
                                   GLsizei bufsize, GLsizei *length,
                                   GLint *size, GLenum *type, GLchar *name);
exposed_to_tests void _gl_get_active_uniform (server_t *server, GLuint program, GLuint index,
                                    GLsizei bufsize, GLsizei *length,
                                    GLint *size, GLenum *type,
                                    GLchar *name);
exposed_to_tests void _gl_get_attached_shaders (server_t *server, GLuint program, GLsizei maxCount,
                                      GLsizei *count, GLuint *shaders);
exposed_to_tests GLint _gl_get_attrib_location (server_t *server, GLuint program, const GLchar *name);
exposed_to_tests void _gl_get_buffer_parameteriv (server_t *server, GLenum target, GLenum value, GLint *data);
exposed_to_tests GLenum _gl_get_error (server_t *server);
exposed_to_tests void _gl_get_framebuffer_attachment_parameteriv (server_t *server, GLenum target,
                                                        GLenum attachment,
                                                        GLenum pname,
                                                        GLint *params);
exposed_to_tests void _gl_get_program_info_log (server_t *server, GLuint program, GLsizei maxLength,
                                      GLsizei *length, GLchar *infoLog);
exposed_to_tests void _gl_get_programiv (server_t *server, GLuint program, GLenum pname, GLint *params);
exposed_to_tests void _gl_get_renderbuffer_parameteriv (server_t *server, GLenum target,
                                              GLenum pname,
                                              GLint *params);
exposed_to_tests void _gl_get_shader_info_log (server_t *server, GLuint program, GLsizei maxLength,
                                     GLsizei *length, GLchar *infoLog);
exposed_to_tests void _gl_get_shader_precision_format (server_t *server, GLenum shaderType,
                                             GLenum precisionType,
                                             GLint *range,
                                             GLint *precision);
exposed_to_tests void _gl_get_shader_source  (server_t *server, GLuint shader, GLsizei bufSize,
                                    GLsizei *length, GLchar *source);
exposed_to_tests void _gl_get_shaderiv (server_t *server, GLuint shader, GLenum pname, GLint *params);
exposed_to_tests const GLubyte *_gl_get_string (server_t *server, GLenum name);
exposed_to_tests void _gl_get_tex_parameteriv (server_t *server, GLenum target, GLenum pname,
                                     GLint *params);
exposed_to_tests void _gl_get_tex_parameterfv (server_t *server, GLenum target, GLenum pname, GLfloat *params);
exposed_to_tests void _gl_get_uniformiv (server_t *server, GLuint program, GLint location,
                               GLint *params);
exposed_to_tests void _gl_get_uniformfv (server_t *server, GLuint program, GLint location,
                               GLfloat *params);
exposed_to_tests GLint _gl_get_uniform_location (server_t *server, GLuint program, const GLchar *name);
exposed_to_tests void _gl_get_vertex_attribfv (server_t *server, GLuint index, GLenum pname,
                                     GLfloat *params);
exposed_to_tests void _gl_get_vertex_attribiv (server_t *server, GLuint index, GLenum pname, GLint *params);
exposed_to_tests void _gl_get_vertex_attrib_pointerv (server_t *server, GLuint index, GLenum pname,
                                            GLvoid **pointer);
exposed_to_tests void _gl_hint (server_t *server, GLenum target, GLenum mode);
exposed_to_tests GLboolean _gl_is_buffer (server_t *server, GLuint buffer);
exposed_to_tests GLboolean _gl_is_enabled (server_t *server, GLenum cap);
exposed_to_tests GLboolean _gl_is_framebuffer (server_t *server, GLuint framebuffer);
exposed_to_tests GLboolean _gl_is_program (server_t *server, GLuint program);
exposed_to_tests GLboolean _gl_is_renderbuffer (server_t *server, GLuint renderbuffer);
exposed_to_tests GLboolean _gl_is_shader (server_t *server, GLuint shader);
exposed_to_tests void _gl_line_width (server_t *server, GLfloat width);
exposed_to_tests GLboolean _gl_is_texture (server_t *server, GLuint texture);
exposed_to_tests void _gl_link_program (server_t *server, GLuint program);
exposed_to_tests void _gl_pixel_storei (server_t *server, GLenum pname, GLint param);
exposed_to_tests void _gl_polygon_offset (server_t *server, GLfloat factor, GLfloat units);
exposed_to_tests void _gl_read_pixels (server_t *server, GLint x, GLint y,
                             GLsizei width, GLsizei height,
                             GLenum format, GLenum type, GLvoid *data);
exposed_to_tests void _gl_compile_shader (server_t *server, GLuint shader);
exposed_to_tests void _gl_release_shader_compiler (server_t *server);
exposed_to_tests void _gl_renderbuffer_storage (server_t *server, GLenum target, GLenum internalformat,
                                      GLsizei width, GLsizei height);
exposed_to_tests void _gl_sample_coverage (server_t *server, GLclampf value, GLboolean invert);
exposed_to_tests void _gl_scissor (server_t *server, GLint x, GLint y, GLsizei width, GLsizei height);
exposed_to_tests void _gl_shader_binary (server_t *server, GLsizei n, const GLuint *shaders,
                               GLenum binaryformat, const void *binary,
                               GLsizei length);
exposed_to_tests void _gl_shader_source (server_t *server, GLuint shader, GLsizei count,
                               const GLchar **string, const GLint *length);
exposed_to_tests void _gl_stencil_func_separate (server_t *server, GLenum face, GLenum func,
                                       GLint ref, GLuint mask);
exposed_to_tests void _gl_stencil_func (server_t *server, GLenum func, GLint ref, GLuint mask);
exposed_to_tests void _gl_stencil_mask_separate (server_t *server, GLenum face, GLuint mask);
exposed_to_tests void _gl_stencil_mask (server_t *server, GLuint mask);
exposed_to_tests void _gl_stencil_op_separate (server_t *server, GLenum face, GLenum sfail,
                                     GLenum dpfail, GLenum dppass);
exposed_to_tests void _gl_stencil_op (server_t *server, GLenum sfail, GLenum dpfail, GLenum dppass);
exposed_to_tests void _gl_tex_image_2d (server_t *server, GLenum target, GLint level,
                              GLint internalformat,
                              GLsizei width, GLsizei height, GLint border,
                              GLenum format, GLenum type,
                              const GLvoid *data);
exposed_to_tests void _gl_tex_parameteri (server_t *server, GLenum target, GLenum pname, GLint param);
exposed_to_tests void _gl_tex_parameterf (server_t *server, GLenum target, GLenum pname, GLfloat param);
exposed_to_tests void _gl_tex_sub_image_2d (server_t *server, GLenum target, GLint level,
                                  GLint xoffset, GLint yoffset,
                                  GLsizei width, GLsizei height,
                                  GLenum format, GLenum type,
                                  const GLvoid *data);
exposed_to_tests void _gl_uniform1f (server_t *server, GLint location, GLfloat v0);
exposed_to_tests void _gl_uniform2f (server_t *server, GLint location, GLfloat v0, GLfloat v1);
exposed_to_tests void _gl_uniform3f (server_t *server, GLint location, GLfloat v0, GLfloat v1,
                           GLfloat v2);
exposed_to_tests void _gl_uniform4f (server_t *server, GLint location, GLfloat v0, GLfloat v1,
                           GLfloat v2, GLfloat v3);
exposed_to_tests void _gl_uniform1i (server_t *server, GLint location, GLint v0);
exposed_to_tests void _gl_uniform_2i (server_t *server, GLint location, GLint v0, GLint v1);
exposed_to_tests void _gl_uniform3i (server_t *server, GLint location, GLint v0, GLint v1, GLint v2);
exposed_to_tests void _gl_uniform4i (server_t *server, GLint location, GLint v0, GLint v1,
                           GLint v2, GLint v3);
exposed_to_tests void
_gl_uniform_fv (server_t *server, int i, GLint location,
                GLsizei count, const GLfloat *value);
exposed_to_tests void _gl_uniform1fv (server_t *server, GLint location, GLsizei count,
                            const GLfloat *value);
exposed_to_tests void _gl_uniform2fv (server_t *server, GLint location, GLsizei count,
                            const GLfloat *value);
exposed_to_tests void _gl_uniform3fv (server_t *server, GLint location, GLsizei count,
                            const GLfloat *value);
exposed_to_tests void _gl_uniform4fv (server_t *server, GLint location, GLsizei count,
                            const GLfloat *value);
exposed_to_tests void
_gl_uniform_iv (server_t *server, int i, GLint location,
                GLsizei count, const GLint *value);
exposed_to_tests void _gl_uniform1iv (server_t *server, GLint location, GLsizei count,
                            const GLint *value);
exposed_to_tests void _gl_uniform2iv (server_t *server, GLint location, GLsizei count,
                            const GLint *value);
exposed_to_tests void _gl_uniform3iv (server_t *server, GLint location, GLsizei count,
                            const GLint *value);
exposed_to_tests void _gl_uniform4iv (server_t *server, GLint location, GLsizei count,
                            const GLint *value);
exposed_to_tests void _gl_use_program (server_t *server, GLuint program);
exposed_to_tests void _gl_validate_program (server_t *server, GLuint program);
exposed_to_tests void _gl_vertex_attrib1f (server_t *server, GLuint index, GLfloat v0);
exposed_to_tests void _gl_vertex_attrib2f (server_t *server, GLuint index, GLfloat v0, GLfloat v1);
exposed_to_tests void _gl_vertex_attrib3f (server_t *server, GLuint index, GLfloat v0,
                                 GLfloat v1, GLfloat v2);
exposed_to_tests void _gl_vertex_attrib4f (server_t *server, GLuint index, GLfloat v0, GLfloat v1,
                                 GLfloat v2, GLfloat v3);
exposed_to_tests void
_gl_vertex_attrib_fv (server_t *server, int i, GLuint index, const GLfloat *v);
exposed_to_tests void _gl_vertex_attrib1fv (server_t *server, GLuint index, const GLfloat *v);
exposed_to_tests void _gl_vertex_attrib2fv (server_t *server, GLuint index, const GLfloat *v);
exposed_to_tests void _gl_vertex_attrib3fv (server_t *server, GLuint index, const GLfloat *v);
exposed_to_tests void _gl_vertex_attrib_pointer (server_t *server, GLuint index, GLint size,
                                       GLenum type, GLboolean normalized,
                                       GLsizei stride, const GLvoid *pointer);
exposed_to_tests void _gl_viewport (server_t *server, GLint x, GLint y, GLsizei width, GLsizei height);
/* end of GLES2 core profile */












#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_GLES_SERVER_TESTS_H */
