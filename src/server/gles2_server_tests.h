#ifndef GPUPROCESS_GLES_SEVER_TESTS_H
#define GPUPROCESS_GLES_SEVER_TESTS_H
#include "gles2_states.h"
#include "command_buffer_server.h"
#include "compiler_private.h"

#ifdef __cplusplus
extern "C" {
#endif

exposed_to_tests void _gl_set_error(command_buffer_server_t *server, GLenum error);
exposed_to_tests bool _gl_is_valid_func (command_buffer_server_t *server, void *func);
exposed_to_tests bool _gl_is_valid_context (command_buffer_server_t *server);
exposed_to_tests void _gl_active_texture (command_buffer_server_t *server, GLenum texture);
exposed_to_tests void _gl_attach_shader (command_buffer_server_t *server, GLuint program, GLuint shader);
exposed_to_tests void _gl_bind_attrib_location (command_buffer_server_t *server, GLuint program, GLuint index, const GLchar *name);
exposed_to_tests void _gl_bind_buffer (command_buffer_server_t *server, GLenum target, GLuint buffer);
exposed_to_tests void _gl_bind_framebuffer (command_buffer_server_t *server, GLenum target, GLuint framebuffer);
exposed_to_tests void _gl_bind_renderbuffer (command_buffer_server_t *server, GLenum target, GLuint renderbuffer);
exposed_to_tests void _gl_bind_texture (command_buffer_server_t *server, GLenum target, GLuint texture);
exposed_to_tests void _gl_blend_color (command_buffer_server_t *server, GLclampf red, GLclampf green,GLclampf blue, GLclampf alpha);
exposed_to_tests void _gl_blend_equation (command_buffer_server_t *server, GLenum mode);
exposed_to_tests void _gl_blend_equation_separate (command_buffer_server_t *server, GLenum modeRGB, GLenum modeAlpha);
exposed_to_tests void _gl_blend_func (command_buffer_server_t *server, GLenum sfactor, GLenum dfactor);
exposed_to_tests void _gl_blend_func_separate (command_buffer_server_t *server, GLenum srcRGB, GLenum dstRGB,GLenum srcAlpha, GLenum dstAlpha);
exposed_to_tests void _gl_buffer_data (command_buffer_server_t *server, GLenum target, GLsizeiptr size,const GLvoid *data, GLenum usage);
exposed_to_tests void _gl_buffer_sub_data (command_buffer_server_t *server, GLenum target, GLintptr offset,GLsizeiptr size, const GLvoid *data);
exposed_to_tests GLenum _gl_check_framebuffer_status (command_buffer_server_t *server, GLenum target);
exposed_to_tests void _gl_clear (command_buffer_server_t *server, GLbitfield mask);
exposed_to_tests void _gl_clear_color (command_buffer_server_t *server, GLclampf red, GLclampf green,GLclampf blue, GLclampf alpha);
exposed_to_tests void _gl_clear_depthf (command_buffer_server_t *server, GLclampf depth);
exposed_to_tests void _gl_clear_stencil (command_buffer_server_t *server, GLint s);
exposed_to_tests void _gl_color_mask (command_buffer_server_t *server, GLboolean red, GLboolean green,GLboolean blue, GLboolean alpha);
exposed_to_tests void _gl_compressed_tex_image_2d (command_buffer_server_t *server, GLenum target, GLint level,
                                         GLenum internalformat,
                                         GLsizei width, GLsizei height,
                                         GLint border, GLsizei imageSize,
                                         const GLvoid *data);
exposed_to_tests void _gl_compressed_tex_sub_image_2d (command_buffer_server_t *server, GLenum target, GLint level,
                                             GLint xoffset, GLint yoffset,
                                             GLsizei width, GLsizei height,
                                             GLenum format,
                                             GLsizei imageSize,
                                             const GLvoid *data);
exposed_to_tests void _gl_copy_tex_image_2d (command_buffer_server_t *server, GLenum target, GLint level,
                                   GLenum internalformat,
                                   GLint x, GLint y,
                                   GLsizei width, GLsizei height,
                                   GLint border);
exposed_to_tests void _gl_copy_tex_sub_image_2d (command_buffer_server_t *server, GLenum target, GLint level,
                                       GLint xoffset, GLint yoffset,
                                       GLint x, GLint y,
                                       GLsizei width, GLsizei height);
exposed_to_tests GLuint _gl_create_program  (command_buffer_server_t *server);
exposed_to_tests GLuint _gl_create_shader (command_buffer_server_t *server, GLenum shaderType);
exposed_to_tests void _gl_cull_face (command_buffer_server_t *server, GLenum mode);
exposed_to_tests void _gl_delete_buffers (command_buffer_server_t *server, GLsizei n, const GLuint *buffers);
exposed_to_tests void _gl_delete_framebuffers (command_buffer_server_t *server, GLsizei n, const GLuint *framebuffers);
exposed_to_tests void _gl_delete_program (command_buffer_server_t *server, GLuint program);
exposed_to_tests void _gl_delete_renderbuffers (command_buffer_server_t *server, GLsizei n, const GLuint *renderbuffers);
exposed_to_tests void _gl_delete_shader (command_buffer_server_t *server, GLuint shader);
exposed_to_tests void _gl_delete_textures (command_buffer_server_t *server, GLsizei n, const GLuint *textures);
exposed_to_tests void _gl_depth_func (command_buffer_server_t *server, GLenum func);
exposed_to_tests void _gl_depth_mask (command_buffer_server_t *server, GLboolean flag);
exposed_to_tests void _gl_depth_rangef (command_buffer_server_t *server, GLclampf nearVal, GLclampf farVal);
exposed_to_tests void _gl_detach_shader (command_buffer_server_t *server, GLuint program, GLuint shader);
exposed_to_tests void _gl_set_cap (command_buffer_server_t *server, GLenum cap, GLboolean enable);
exposed_to_tests void _gl_disable (command_buffer_server_t *server, GLenum cap);
exposed_to_tests void _gl_enable (command_buffer_server_t *server, GLenum cap);
exposed_to_tests bool _gl_index_is_too_large (command_buffer_server_t *server, gles2_state_t *state, GLuint index);
exposed_to_tests void _gl_set_vertex_attrib_array (command_buffer_server_t *server, GLuint index, gles2_state_t *state,GLboolean enable);
exposed_to_tests void _gl_disable_vertex_attrib_array (command_buffer_server_t *server, GLuint index);
exposed_to_tests void _gl_enable_vertex_attrib_array (command_buffer_server_t *server, GLuint index);
exposed_to_tests char *_gl_create_data_array (command_buffer_server_t *server, vertex_attrib_t *attrib, int count);
exposed_to_tests void _gl_draw_arrays (command_buffer_server_t *server, GLenum mode, GLint first, GLsizei count);
exposed_to_tests char *_gl_create_indices_array (command_buffer_server_t *server, GLenum mode, GLenum type, int count,char *indices);
exposed_to_tests void _gl_draw_elements (command_buffer_server_t *server, GLenum mode, GLsizei count, GLenum type,const GLvoid *indices);
exposed_to_tests void _gl_finish (command_buffer_server_t *server);
exposed_to_tests void _gl_flush (command_buffer_server_t *server);
exposed_to_tests void _gl_framebuffer_renderbuffer (command_buffer_server_t *server, GLenum target, GLenum attachment,
                                          GLenum renderbuffertarget,
                                          GLenum renderbuffer);
exposed_to_tests void _gl_framebuffer_texture_2d (command_buffer_server_t *server, GLenum target, GLenum attachment,
                                        GLenum textarget, GLuint texture,
                                        GLint level);
exposed_to_tests void _gl_front_face (command_buffer_server_t *server, GLenum mode);
exposed_to_tests void _gl_gen_buffers (command_buffer_server_t *server, GLsizei n, GLuint *buffers);
exposed_to_tests void _gl_gen_framebuffers (command_buffer_server_t *server, GLsizei n, GLuint *framebuffers);
exposed_to_tests void _gl_gen_renderbuffers (command_buffer_server_t *server, GLsizei n, GLuint *renderbuffers);
exposed_to_tests void _gl_gen_textures (command_buffer_server_t *server, GLsizei n, GLuint *textures);
exposed_to_tests void _gl_generate_mipmap (command_buffer_server_t *server, GLenum target);
exposed_to_tests void _gl_get_booleanv (command_buffer_server_t *server, GLenum pname, GLboolean *params);
exposed_to_tests void _gl_get_floatv (command_buffer_server_t *server, GLenum pname, GLfloat *params);
exposed_to_tests void _gl_get_integerv (command_buffer_server_t *server, GLenum pname, GLint *params);
exposed_to_tests void _gl_get_active_attrib (command_buffer_server_t *server, GLuint program, GLuint index,
                                   GLsizei bufsize, GLsizei *length,
                                   GLint *size, GLenum *type, GLchar *name);
exposed_to_tests void _gl_get_active_uniform (command_buffer_server_t *server, GLuint program, GLuint index,
                                    GLsizei bufsize, GLsizei *length,
                                    GLint *size, GLenum *type,
                                    GLchar *name);
exposed_to_tests void _gl_get_attached_shaders (command_buffer_server_t *server, GLuint program, GLsizei maxCount,
                                      GLsizei *count, GLuint *shaders);
exposed_to_tests GLint _gl_get_attrib_location (command_buffer_server_t *server, GLuint program, const GLchar *name);
exposed_to_tests void _gl_get_buffer_parameteriv (command_buffer_server_t *server, GLenum target, GLenum value, GLint *data);
exposed_to_tests GLenum _gl_get_error (command_buffer_server_t *server);
exposed_to_tests void _gl_get_framebuffer_attachment_parameteriv (command_buffer_server_t *server, GLenum target,
                                                        GLenum attachment,
                                                        GLenum pname,
                                                        GLint *params);
exposed_to_tests void _gl_get_program_info_log (command_buffer_server_t *server, GLuint program, GLsizei maxLength,
                                      GLsizei *length, GLchar *infoLog);
exposed_to_tests void _gl_get_programiv (command_buffer_server_t *server, GLuint program, GLenum pname, GLint *params);
exposed_to_tests void _gl_get_renderbuffer_parameteriv (command_buffer_server_t *server, GLenum target,
                                              GLenum pname,
                                              GLint *params);
exposed_to_tests void _gl_get_shader_info_log (command_buffer_server_t *server, GLuint program, GLsizei maxLength,
                                     GLsizei *length, GLchar *infoLog);
exposed_to_tests void _gl_get_shader_precision_format (command_buffer_server_t *server, GLenum shaderType,
                                             GLenum precisionType,
                                             GLint *range,
                                             GLint *precision);
exposed_to_tests void _gl_get_shader_source  (command_buffer_server_t *server, GLuint shader, GLsizei bufSize,
                                    GLsizei *length, GLchar *source);
exposed_to_tests void _gl_get_shaderiv (command_buffer_server_t *server, GLuint shader, GLenum pname, GLint *params);
exposed_to_tests const GLubyte *_gl_get_string (command_buffer_server_t *server, GLenum name);
exposed_to_tests void _gl_get_tex_parameteriv (command_buffer_server_t *server, GLenum target, GLenum pname,
                                     GLint *params);
exposed_to_tests void _gl_get_tex_parameterfv (command_buffer_server_t *server, GLenum target, GLenum pname, GLfloat *params);
exposed_to_tests void _gl_get_uniformiv (command_buffer_server_t *server, GLuint program, GLint location,
                               GLint *params);
exposed_to_tests void _gl_get_uniformfv (command_buffer_server_t *server, GLuint program, GLint location,
                               GLfloat *params);
exposed_to_tests GLint _gl_get_uniform_location (command_buffer_server_t *server, GLuint program, const GLchar *name);
exposed_to_tests void _gl_get_vertex_attribfv (command_buffer_server_t *server, GLuint index, GLenum pname,
                                     GLfloat *params);
exposed_to_tests void _gl_get_vertex_attribiv (command_buffer_server_t *server, GLuint index, GLenum pname, GLint *params);
exposed_to_tests void _gl_get_vertex_attrib_pointerv (command_buffer_server_t *server, GLuint index, GLenum pname,
                                            GLvoid **pointer);
exposed_to_tests void _gl_hint (command_buffer_server_t *server, GLenum target, GLenum mode);
exposed_to_tests GLboolean _gl_is_buffer (command_buffer_server_t *server, GLuint buffer);
exposed_to_tests GLboolean _gl_is_enabled (command_buffer_server_t *server, GLenum cap);
exposed_to_tests GLboolean _gl_is_framebuffer (command_buffer_server_t *server, GLuint framebuffer);
exposed_to_tests GLboolean _gl_is_program (command_buffer_server_t *server, GLuint program);
exposed_to_tests GLboolean _gl_is_renderbuffer (command_buffer_server_t *server, GLuint renderbuffer);
exposed_to_tests GLboolean _gl_is_shader (command_buffer_server_t *server, GLuint shader);
exposed_to_tests void _gl_line_width (command_buffer_server_t *server, GLfloat width);
exposed_to_tests GLboolean _gl_is_texture (command_buffer_server_t *server, GLuint texture);
exposed_to_tests void _gl_link_program (command_buffer_server_t *server, GLuint program);
exposed_to_tests void _gl_pixel_storei (command_buffer_server_t *server, GLenum pname, GLint param);
exposed_to_tests void _gl_polygon_offset (command_buffer_server_t *server, GLfloat factor, GLfloat units);
exposed_to_tests void _gl_read_pixels (command_buffer_server_t *server, GLint x, GLint y,
                             GLsizei width, GLsizei height,
                             GLenum format, GLenum type, GLvoid *data);
exposed_to_tests void _gl_compile_shader (command_buffer_server_t *server, GLuint shader);
exposed_to_tests void _gl_release_shader_compiler (command_buffer_server_t *server);
exposed_to_tests void _gl_renderbuffer_storage (command_buffer_server_t *server, GLenum target, GLenum internalformat,
                                      GLsizei width, GLsizei height);
exposed_to_tests void _gl_sample_coverage (command_buffer_server_t *server, GLclampf value, GLboolean invert);
exposed_to_tests void _gl_scissor (command_buffer_server_t *server, GLint x, GLint y, GLsizei width, GLsizei height);
exposed_to_tests void _gl_shader_binary (command_buffer_server_t *server, GLsizei n, const GLuint *shaders,
                               GLenum binaryformat, const void *binary,
                               GLsizei length);
exposed_to_tests void _gl_shader_source (command_buffer_server_t *server, GLuint shader, GLsizei count,
                               const GLchar **string, const GLint *length);
exposed_to_tests void _gl_stencil_func_separate (command_buffer_server_t *server, GLenum face, GLenum func,
                                       GLint ref, GLuint mask);
exposed_to_tests void _gl_stencil_func (command_buffer_server_t *server, GLenum func, GLint ref, GLuint mask);
exposed_to_tests void _gl_stencil_mask_separate (command_buffer_server_t *server, GLenum face, GLuint mask);
exposed_to_tests void _gl_stencil_mask (command_buffer_server_t *server, GLuint mask);
exposed_to_tests void _gl_stencil_op_separate (command_buffer_server_t *server, GLenum face, GLenum sfail,
                                     GLenum dpfail, GLenum dppass);
exposed_to_tests void _gl_stencil_op (command_buffer_server_t *server, GLenum sfail, GLenum dpfail, GLenum dppass);
exposed_to_tests void _gl_tex_image_2d (command_buffer_server_t *server, GLenum target, GLint level,
                              GLint internalformat,
                              GLsizei width, GLsizei height, GLint border,
                              GLenum format, GLenum type,
                              const GLvoid *data);
exposed_to_tests void _gl_tex_parameteri (command_buffer_server_t *server, GLenum target, GLenum pname, GLint param);
exposed_to_tests void _gl_tex_parameterf (command_buffer_server_t *server, GLenum target, GLenum pname, GLfloat param);
exposed_to_tests void _gl_tex_sub_image_2d (command_buffer_server_t *server, GLenum target, GLint level,
                                  GLint xoffset, GLint yoffset,
                                  GLsizei width, GLsizei height,
                                  GLenum format, GLenum type,
                                  const GLvoid *data);
exposed_to_tests void _gl_uniform1f (command_buffer_server_t *server, GLint location, GLfloat v0);
exposed_to_tests void _gl_uniform2f (command_buffer_server_t *server, GLint location, GLfloat v0, GLfloat v1);
exposed_to_tests void _gl_uniform3f (command_buffer_server_t *server, GLint location, GLfloat v0, GLfloat v1,
                           GLfloat v2);
exposed_to_tests void _gl_uniform4f (command_buffer_server_t *server, GLint location, GLfloat v0, GLfloat v1,
                           GLfloat v2, GLfloat v3);
exposed_to_tests void _gl_uniform1i (command_buffer_server_t *server, GLint location, GLint v0);
exposed_to_tests void _gl_uniform_2i (command_buffer_server_t *server, GLint location, GLint v0, GLint v1);
exposed_to_tests void _gl_uniform3i (command_buffer_server_t *server, GLint location, GLint v0, GLint v1, GLint v2);
exposed_to_tests void _gl_uniform4i (command_buffer_server_t *server, GLint location, GLint v0, GLint v1,
                           GLint v2, GLint v3);
exposed_to_tests void
_gl_uniform_fv (command_buffer_server_t *server, int i, GLint location,
                GLsizei count, const GLfloat *value);
exposed_to_tests void _gl_uniform1fv (command_buffer_server_t *server, GLint location, GLsizei count,
                            const GLfloat *value);
exposed_to_tests void _gl_uniform2fv (command_buffer_server_t *server, GLint location, GLsizei count,
                            const GLfloat *value);
exposed_to_tests void _gl_uniform3fv (command_buffer_server_t *server, GLint location, GLsizei count,
                            const GLfloat *value);
exposed_to_tests void _gl_uniform4fv (command_buffer_server_t *server, GLint location, GLsizei count,
                            const GLfloat *value);
exposed_to_tests void
_gl_uniform_iv (command_buffer_server_t *server, int i, GLint location,
                GLsizei count, const GLint *value);
exposed_to_tests void _gl_uniform1iv (command_buffer_server_t *server, GLint location, GLsizei count,
                            const GLint *value);
exposed_to_tests void _gl_uniform2iv (command_buffer_server_t *server, GLint location, GLsizei count,
                            const GLint *value);
exposed_to_tests void _gl_uniform3iv (command_buffer_server_t *server, GLint location, GLsizei count,
                            const GLint *value);
exposed_to_tests void _gl_uniform4iv (command_buffer_server_t *server, GLint location, GLsizei count,
                            const GLint *value);
exposed_to_tests void _gl_use_program (command_buffer_server_t *server, GLuint program);
exposed_to_tests void _gl_validate_program (command_buffer_server_t *server, GLuint program);
exposed_to_tests void _gl_vertex_attrib1f (command_buffer_server_t *server, GLuint index, GLfloat v0);
exposed_to_tests void _gl_vertex_attrib2f (command_buffer_server_t *server, GLuint index, GLfloat v0, GLfloat v1);
exposed_to_tests void _gl_vertex_attrib3f (command_buffer_server_t *server, GLuint index, GLfloat v0,
                                 GLfloat v1, GLfloat v2);
exposed_to_tests void _gl_vertex_attrib4f (command_buffer_server_t *server, GLuint index, GLfloat v0, GLfloat v1,
                                 GLfloat v2, GLfloat v3);
exposed_to_tests void
_gl_vertex_attrib_fv (command_buffer_server_t *server, int i, GLuint index, const GLfloat *v);
exposed_to_tests void _gl_vertex_attrib1fv (command_buffer_server_t *server, GLuint index, const GLfloat *v);
exposed_to_tests void _gl_vertex_attrib2fv (command_buffer_server_t *server, GLuint index, const GLfloat *v);
exposed_to_tests void _gl_vertex_attrib3fv (command_buffer_server_t *server, GLuint index, const GLfloat *v);
exposed_to_tests void _gl_vertex_attrib_pointer (command_buffer_server_t *server, GLuint index, GLint size,
                                       GLenum type, GLboolean normalized,
                                       GLsizei stride, const GLvoid *pointer);
exposed_to_tests void _gl_viewport (command_buffer_server_t *server, GLint x, GLint y, GLsizei width, GLsizei height);
/* end of GLES2 core profile */












#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_GLES_SERVER_TESTS_H */
