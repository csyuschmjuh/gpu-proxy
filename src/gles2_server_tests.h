#ifndef GPUPROCESS_GLES_SEVER_TESTS_H
#define GPUPROCESS_GLES_SEVER_TESTS_H
#include <GL/glx.h>
#include <GL/glxext.h>
#include "gles2_states.h"
#include "compiler_private.h"

#ifdef __cplusplus
extern "C" {
#endif

exposed_to_tests void _gl_set_error(GLenum error);
exposed_to_tests bool _gl_is_valid_func (void *func);
exposed_to_tests bool _gl_is_valid_context (void);
exposed_to_tests void _gl_active_texture (GLenum texture);
exposed_to_tests void _gl_attach_shader (GLuint program, GLuint shader);
exposed_to_tests void _gl_bind_attrib_location (GLuint program, GLuint index, const GLchar *name);
exposed_to_tests void _gl_bind_buffer (GLenum target, GLuint buffer);
exposed_to_tests void _gl_bind_framebuffer (GLenum target, GLuint framebuffer);
exposed_to_tests void _gl_bind_renderbuffer (GLenum target, GLuint renderbuffer);
exposed_to_tests void _gl_bind_texture (GLenum target, GLuint texture);
exposed_to_tests void _gl_blend_color (GLclampf red, GLclampf green,GLclampf blue, GLclampf alpha);
exposed_to_tests void _gl_blend_equation (GLenum mode);
exposed_to_tests void _gl_blend_equation_separate (GLenum modeRGB, GLenum modeAlpha);
exposed_to_tests void _gl_blend_func (GLenum sfactor, GLenum dfactor);
exposed_to_tests void _gl_blend_func_separate (GLenum srcRGB, GLenum dstRGB,GLenum srcAlpha, GLenum dstAlpha);
exposed_to_tests void _gl_buffer_data (GLenum target, GLsizeiptr size,const GLvoid *data, GLenum usage);
exposed_to_tests void _gl_buffer_sub_data (GLenum target, GLintptr offset,GLsizeiptr size, const GLvoid *data);
exposed_to_tests GLenum _gl_check_framebuffer_status (GLenum target);
exposed_to_tests void _gl_clear (GLbitfield mask);
exposed_to_tests void _gl_clear_color (GLclampf red, GLclampf green,GLclampf blue, GLclampf alpha);
exposed_to_tests void _gl_clear_depthf (GLclampf depth);
exposed_to_tests void _gl_clear_stencil (GLint s);
exposed_to_tests void _gl_color_mask (GLboolean red, GLboolean green,GLboolean blue, GLboolean alpha);
exposed_to_tests void _gl_compressed_tex_image_2d (GLenum target, GLint level,
                                         GLenum internalformat,
                                         GLsizei width, GLsizei height,
                                         GLint border, GLsizei imageSize,
                                         const GLvoid *data);
exposed_to_tests void _gl_compressed_tex_sub_image_2d (GLenum target, GLint level,
                                             GLint xoffset, GLint yoffset,
                                             GLsizei width, GLsizei height,
                                             GLenum format,
                                             GLsizei imageSize,
                                             const GLvoid *data);
exposed_to_tests void _gl_copy_tex_image_2d (GLenum target, GLint level,
                                   GLenum internalformat,
                                   GLint x, GLint y,
                                   GLsizei width, GLsizei height,
                                   GLint border);
exposed_to_tests void _gl_copy_tex_sub_image_2d (GLenum target, GLint level,
                                       GLint xoffset, GLint yoffset,
                                       GLint x, GLint y,
                                       GLsizei width, GLsizei height);
exposed_to_tests GLuint _gl_create_program  (void);
exposed_to_tests GLuint _gl_create_shader (GLenum shaderType);
exposed_to_tests void _gl_cull_face (GLenum mode);
exposed_to_tests void _gl_delete_buffers (GLsizei n, const GLuint *buffers);
exposed_to_tests void _gl_delete_framebuffers (GLsizei n, const GLuint *framebuffers);
exposed_to_tests void _gl_delete_program (GLuint program);
exposed_to_tests void _gl_delete_renderbuffers (GLsizei n, const GLuint *renderbuffers);
exposed_to_tests void _gl_delete_shader (GLuint shader);
exposed_to_tests void _gl_delete_textures (GLsizei n, const GLuint *textures);
exposed_to_tests void _gl_depth_func (GLenum func);
exposed_to_tests void _gl_depth_mask (GLboolean flag);
exposed_to_tests void _gl_depth_rangef (GLclampf nearVal, GLclampf farVal);
exposed_to_tests void _gl_detach_shader (GLuint program, GLuint shader);
exposed_to_tests void _gl_set_cap (GLenum cap, GLboolean enable);
exposed_to_tests void _gl_disable (GLenum cap);
exposed_to_tests void _gl_enable (GLenum cap);
exposed_to_tests bool _gl_index_is_too_large (gles2_state_t *state, GLuint index);
exposed_to_tests void _gl_set_vertex_attrib_array (GLuint index, gles2_state_t *state,GLboolean enable);
exposed_to_tests void _gl_disable_vertex_attrib_array (GLuint index);
exposed_to_tests void _gl_enable_vertex_attrib_array (GLuint index);
exposed_to_tests char *_gl_create_data_array (vertex_attrib_t *attrib, int count);
exposed_to_tests void _gl_draw_arrays (GLenum mode, GLint first, GLsizei count);
exposed_to_tests char *_gl_create_indices_array (GLenum mode, GLenum type, int count,char *indices);
exposed_to_tests void _gl_draw_elements (GLenum mode, GLsizei count, GLenum type,const GLvoid *indices);
exposed_to_tests void _gl_finish (void);
exposed_to_tests void _gl_flush (void);
exposed_to_tests void _gl_framebuffer_renderbuffer (GLenum target, GLenum attachment,
                                          GLenum renderbuffertarget,
                                          GLenum renderbuffer);
exposed_to_tests void _gl_framebuffer_texture_2d (GLenum target, GLenum attachment,
                                        GLenum textarget, GLuint texture,
                                        GLint level);
exposed_to_tests void _gl_front_face (GLenum mode);
exposed_to_tests void _gl_gen_buffers (GLsizei n, GLuint *buffers);
exposed_to_tests void _gl_gen_framebuffers (GLsizei n, GLuint *framebuffers);
exposed_to_tests void _gl_gen_renderbuffers (GLsizei n, GLuint *renderbuffers);
exposed_to_tests void _gl_gen_textures (GLsizei n, GLuint *textures);
exposed_to_tests void _gl_generate_mipmap (GLenum target);
exposed_to_tests void _gl_get_booleanv (GLenum pname, GLboolean *params);
exposed_to_tests void _gl_get_floatv (GLenum pname, GLfloat *params);
exposed_to_tests void _gl_get_integerv (GLenum pname, GLint *params);
exposed_to_tests void _gl_get_active_attrib (GLuint program, GLuint index,
                                   GLsizei bufsize, GLsizei *length,
                                   GLint *size, GLenum *type, GLchar *name);
exposed_to_tests void _gl_get_active_uniform (GLuint program, GLuint index,
                                    GLsizei bufsize, GLsizei *length,
                                    GLint *size, GLenum *type,
                                    GLchar *name);
exposed_to_tests void _gl_get_attached_shaders (GLuint program, GLsizei maxCount,
                                      GLsizei *count, GLuint *shaders);
exposed_to_tests GLint _gl_get_attrib_location (GLuint program, const GLchar *name);
exposed_to_tests void _gl_get_buffer_parameteriv (GLenum target, GLenum value, GLint *data);
exposed_to_tests GLenum _gl_get_error (void);
exposed_to_tests void _gl_get_framebuffer_attachment_parameteriv (GLenum target,
                                                        GLenum attachment,
                                                        GLenum pname,
                                                        GLint *params);
exposed_to_tests void _gl_get_program_info_log (GLuint program, GLsizei maxLength,
                                      GLsizei *length, GLchar *infoLog);
exposed_to_tests void _gl_get_programiv (GLuint program, GLenum pname, GLint *params);
exposed_to_tests void _gl_get_renderbuffer_parameteriv (GLenum target,
                                              GLenum pname,
                                              GLint *params);
exposed_to_tests void _gl_get_shader_info_log (GLuint program, GLsizei maxLength,
                                     GLsizei *length, GLchar *infoLog);
exposed_to_tests void _gl_get_shader_precision_format (GLenum shaderType,
                                             GLenum precisionType,
                                             GLint *range,
                                             GLint *precision);
exposed_to_tests void _gl_get_shader_source  (GLuint shader, GLsizei bufSize,
                                    GLsizei *length, GLchar *source);
exposed_to_tests void _gl_get_shaderiv (GLuint shader, GLenum pname, GLint *params);
exposed_to_tests const GLubyte *_gl_get_string (GLenum name);
exposed_to_tests void _gl_get_tex_parameteriv (GLenum target, GLenum pname,
                                     GLint *params);
exposed_to_tests void _gl_get_tex_parameterfv (GLenum target, GLenum pname, GLfloat *params);
exposed_to_tests void _gl_get_uniformiv (GLuint program, GLint location,
                               GLint *params);
exposed_to_tests void _gl_get_uniformfv (GLuint program, GLint location,
                               GLfloat *params);
exposed_to_tests GLint _gl_get_uniform_location (GLuint program, const GLchar *name);
exposed_to_tests void _gl_get_vertex_attribfv (GLuint index, GLenum pname,
                                     GLfloat *params);
exposed_to_tests void _gl_get_vertex_attribiv (GLuint index, GLenum pname, GLint *params);
exposed_to_tests void _gl_get_vertex_attrib_pointerv (GLuint index, GLenum pname,
                                            GLvoid **pointer);
exposed_to_tests void _gl_hint (GLenum target, GLenum mode);
exposed_to_tests GLboolean _gl_is_buffer (GLuint buffer);
exposed_to_tests GLboolean _gl_is_enabled (GLenum cap);
exposed_to_tests GLboolean _gl_is_framebuffer (GLuint framebuffer);
exposed_to_tests GLboolean _gl_is_program (GLuint program);
exposed_to_tests GLboolean _gl_is_renderbuffer (GLuint renderbuffer);
exposed_to_tests GLboolean _gl_is_shader (GLuint shader);
exposed_to_tests void _gl_line_width (GLfloat width);
exposed_to_tests GLboolean _gl_is_texture (GLuint texture);
exposed_to_tests void _gl_link_program (GLuint program);
exposed_to_tests void _gl_pixel_storei (GLenum pname, GLint param);
exposed_to_tests void _gl_polygon_offset (GLfloat factor, GLfloat units);
exposed_to_tests void _gl_read_pixels (GLint x, GLint y,
                             GLsizei width, GLsizei height,
                             GLenum format, GLenum type, GLvoid *data);
exposed_to_tests void _gl_compile_shader (GLuint shader);
exposed_to_tests void _gl_release_shader_compiler (void);
exposed_to_tests void _gl_renderbuffer_storage (GLenum target, GLenum internalformat,
                                      GLsizei width, GLsizei height);
exposed_to_tests void _gl_sample_coverage (GLclampf value, GLboolean invert);
exposed_to_tests void _gl_scissor (GLint x, GLint y, GLsizei width, GLsizei height);
exposed_to_tests void _gl_shader_binary (GLsizei n, const GLuint *shaders,
                               GLenum binaryformat, const void *binary,
                               GLsizei length);
exposed_to_tests void _gl_shader_source (GLuint shader, GLsizei count,
                               const GLchar **string, const GLint *length);
exposed_to_tests void _gl_stencil_func_separate (GLenum face, GLenum func,
                                       GLint ref, GLuint mask);
exposed_to_tests void _gl_stencil_func (GLenum func, GLint ref, GLuint mask);
exposed_to_tests void _gl_stencil_mask_separate (GLenum face, GLuint mask);
exposed_to_tests void _gl_stencil_mask (GLuint mask);
exposed_to_tests void _gl_stencil_op_separate (GLenum face, GLenum sfail,
                                     GLenum dpfail, GLenum dppass);
exposed_to_tests void _gl_stencil_op (GLenum sfail, GLenum dpfail, GLenum dppass);
exposed_to_tests void _gl_tex_image_2d (GLenum target, GLint level,
                              GLint internalformat,
                              GLsizei width, GLsizei height, GLint border,
                              GLenum format, GLenum type,
                              const GLvoid *data);
exposed_to_tests void _gl_tex_parameteri (GLenum target, GLenum pname, GLint param);
exposed_to_tests void _gl_tex_parameterf (GLenum target, GLenum pname, GLfloat param);
exposed_to_tests void _gl_tex_sub_image_2d (GLenum target, GLint level,
                                  GLint xoffset, GLint yoffset,
                                  GLsizei width, GLsizei height,
                                  GLenum format, GLenum type,
                                  const GLvoid *data);
exposed_to_tests void _gl_uniform1f (GLint location, GLfloat v0);
exposed_to_tests void _gl_uniform2f (GLint location, GLfloat v0, GLfloat v1);
exposed_to_tests void _gl_uniform3f (GLint location, GLfloat v0, GLfloat v1,
                           GLfloat v2);
exposed_to_tests void _gl_uniform4f (GLint location, GLfloat v0, GLfloat v1,
                           GLfloat v2, GLfloat v3);
exposed_to_tests void _gl_uniform1i (GLint location, GLint v0);
exposed_to_tests void _gl_uniform_2i (GLint location, GLint v0, GLint v1);
exposed_to_tests void _gl_uniform3i (GLint location, GLint v0, GLint v1, GLint v2);
exposed_to_tests void _gl_uniform4i (GLint location, GLint v0, GLint v1,
                           GLint v2, GLint v3);
exposed_to_tests void
_gl_uniform_fv (int i, GLint location,
                GLsizei count, const GLfloat *value);
exposed_to_tests void _gl_uniform1fv (GLint location, GLsizei count,
                            const GLfloat *value);
exposed_to_tests void _gl_uniform2fv (GLint location, GLsizei count,
                            const GLfloat *value);
exposed_to_tests void _gl_uniform3fv (GLint location, GLsizei count,
                            const GLfloat *value);
exposed_to_tests void _gl_uniform4fv (GLint location, GLsizei count,
                            const GLfloat *value);
exposed_to_tests void
_gl_uniform_iv (int i, GLint location,
                GLsizei count, const GLint *value);
exposed_to_tests void _gl_uniform1iv (GLint location, GLsizei count,
                            const GLint *value);
exposed_to_tests void _gl_uniform2iv (GLint location, GLsizei count,
                            const GLint *value);
exposed_to_tests void _gl_uniform3iv (GLint location, GLsizei count,
                            const GLint *value);
exposed_to_tests void _gl_uniform4iv (GLint location, GLsizei count,
                            const GLint *value);
exposed_to_tests void _gl_use_program (GLuint program);
exposed_to_tests void _gl_validate_program (GLuint program);
exposed_to_tests void _gl_vertex_attrib1f (GLuint index, GLfloat v0);
exposed_to_tests void _gl_vertex_attrib2f (GLuint index, GLfloat v0, GLfloat v1);
exposed_to_tests void _gl_vertex_attrib3f (GLuint index, GLfloat v0,
                                 GLfloat v1, GLfloat v2);
exposed_to_tests void _gl_vertex_attrib4f (GLuint index, GLfloat v0, GLfloat v1,
                                 GLfloat v2, GLfloat v3);
exposed_to_tests void
_gl_vertex_attrib_fv (int i, GLuint index, const GLfloat *v);
exposed_to_tests void _gl_vertex_attrib1fv (GLuint index, const GLfloat *v);
exposed_to_tests void _gl_vertex_attrib2fv (GLuint index, const GLfloat *v);
exposed_to_tests void _gl_vertex_attrib3fv (GLuint index, const GLfloat *v);
exposed_to_tests void _gl_vertex_attrib_pointer (GLuint index, GLint size,
                                       GLenum type, GLboolean normalized,
                                       GLsizei stride, const GLvoid *pointer);
exposed_to_tests void _gl_viewport (GLint x, GLint y, GLsizei width, GLsizei height);
/* end of GLES2 core profile */












#ifdef __cplusplus
}
#endif

#endif /* GPUPROCESS_GLES_SERVER_TESTS_H */
