#ifndef GPUPROCESS_COMMAND_CUSTOM_H
#define GPUPROCESS_COMMAND_CUSTOM_H

void command_glteximage2d_init_custom (command_t *abstract_command,
                                       GLenum target,
                                       GLint level,
                                       GLint internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLint border,
                                       GLenum format,
                                       GLenum type,
                                       const void* pixels,
                                       int unpack_alignment);

void
command_gltexsubimage2d_init_custom (command_t *abstract_command,
                                     GLenum target,
                                     GLint level,
                                     GLint xoffset,
                                     GLint yoffset,
                                     GLsizei width,
                                     GLsizei height,
                                     GLenum format,
                                     GLenum type,
                                     const void* pixels,
                                     int unpack_alignment);
#endif
