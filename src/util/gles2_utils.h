#include "compiler_private.h"
#include <EGL/egl.h>
#include <stdint.h>
#include <stdbool.h>

private void
copy_rect_to_buffer (const void *pixels,
                     void *buffer,
                     int format,
                     int type,
                     uint32_t height,
                     uint32_t unpack_skip_pixels,
                     uint32_t unpack_skip_rows,
                     uint32_t unpadded_row_size,
                     uint32_t pixels_padded_row_size,
                     uint32_t buffer_padded_row_size);

private bool
compute_image_data_sizes (int width,
                          int height,
                          int format,
                          int type,
                          int unpack_alignment,
                          int unpack_row_length,
                          int unpack_skip_rows,
                          uint32_t *size,
                          uint32_t *ret_unpadded_row_size,
                          uint32_t *ret_padded_row_size);

private uint32_t
compute_image_group_size (int format,
                          int type);

private size_t
_get_egl_attrib_list_size (const EGLint *attrib_list);

