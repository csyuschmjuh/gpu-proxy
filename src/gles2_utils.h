#include "compiler_private.h"
#include <stdint.h>
#include <stdbool.h>

private void
copy_rect_to_buffer (const void *pixels,
                     void *buffer,
                     uint32_t height,
                     uint32_t unpadded_row_size,
                     uint32_t pixels_padded_row_size,
                     bool flip_y,
                     uint32_t buffer_padded_row_size);

bool
compute_image_padded_row_size (int width,
                               int format,
                               int type,
                               int unpack_alignment,
                               uint32_t *padded_row_size);

bool
compute_image_data_size (int width,
                         int height,
                         int format,
                         int type,
                         int unpack_alignment,
                         uint32_t *size,
                         uint32_t *ret_unpadded_row_size,
                         uint32_t *ret_padded_row_size);
