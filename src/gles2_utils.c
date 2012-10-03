/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "gles2_utils.h"
#include <stdlib.h>
#include <string.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

inline bool
safe_multiply (uint32_t a,
               uint32_t b,
               uint32_t *dst)
{
  if (b == 0) {
    *dst = 0;
    return true;
  }
  uint32_t v = a * b;
  if (v / b != a) {
    *dst = 0;
    return false;
  }
  *dst = v;
  return true;
}

inline bool
safe_add (uint32_t a,
          uint32_t b,
          uint32_t *dst)
{
  if (a + b < a) {
    *dst = 0;
    return false;
  }
  *dst = a + b;
  return true;
}

void
copy_rect_to_buffer (const void *pixels,
                     void *buffer,
                     uint32_t height,
                     uint32_t unpadded_row_size,
                     uint32_t pixels_padded_row_size,
                     bool flip_y,
                     uint32_t buffer_padded_row_size)
{
    const char *source = (const char *) pixels;
    char *dest = (char *) buffer;
    if (flip_y || pixels_padded_row_size != buffer_padded_row_size) {
        if (flip_y)
            dest += buffer_padded_row_size * (height - 1);

        // The last row is copied unpadded at the end.
        for (; height > 1; --height) {
            memcpy(dest, source, buffer_padded_row_size);
            if (flip_y)
                dest -= buffer_padded_row_size;
            else
                dest += buffer_padded_row_size;

            source += pixels_padded_row_size;
        }
        memcpy(dest, source, unpadded_row_size);
  } else {
      uint32_t size = (height - 1) * pixels_padded_row_size + unpadded_row_size;
      memcpy(dest, source, size);
  }
}

// Returns the amount of data glTexImage2D or glTexSubImage2D will access.
bool
compute_image_data_sizes (int width, 
                          int height,
                          int format,
                          int type,
                          int unpack_alignment,
                          uint32_t *size,
                          uint32_t *ret_unpadded_row_size,
                          uint32_t *ret_padded_row_size)
{
    uint32_t bytes_per_group = compute_image_group_size(format, type);
    uint32_t row_size;
    if (! safe_multiply (width, bytes_per_group, &row_size))
        return false;

    if (height > 1) {
        uint32_t temp;
          if (! safe_add (row_size, unpack_alignment - 1, &temp))
              return false;
        uint32_t padded_row_size = (temp / unpack_alignment) * unpack_alignment;
        uint32_t size_of_all_but_last_row;

        if (! safe_multiply ((height - 1), padded_row_size, &size_of_all_but_last_row))
            return false;

        if (! safe_add (size_of_all_but_last_row, row_size, size))
            return false;

        if (ret_padded_row_size)
            *ret_padded_row_size = padded_row_size;
    } else {
        if (! safe_multiply (height, row_size, size))
            return false;

        if (ret_padded_row_size)
            *ret_padded_row_size = row_size;
    }

    if (ret_unpadded_row_size)
      *ret_unpadded_row_size = row_size;

  return true;
}

static int
elements_per_group (int format,
                    int type)
{
    switch (type) {
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_UNSIGNED_INT_24_8_OES:
         return 1;
    default:
         break;
    }

    switch (format) {
    case GL_RGB:
        return 3;
    case GL_LUMINANCE_ALPHA:
         return 2;
    case GL_RGBA:
    case GL_BGRA_EXT:
         return 4;
    case GL_ALPHA:
    case GL_LUMINANCE:
    case GL_DEPTH_COMPONENT:
    case GL_DEPTH_COMPONENT24_OES:
    case GL_DEPTH_COMPONENT32_OES:
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH24_STENCIL8_OES:
    case GL_DEPTH_STENCIL_OES:
         return 1;
    default:
         return 0;
    }
}

static int
bytes_per_element (int type)
{
    switch (type) {
    case GL_FLOAT:
    case GL_UNSIGNED_INT_24_8_OES:
    case GL_UNSIGNED_INT:
        return 4;
    case GL_HALF_FLOAT_OES:
    case GL_UNSIGNED_SHORT:
    case GL_SHORT:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
         return 2;
    case GL_UNSIGNED_BYTE:
    case GL_BYTE:
         return 1;
    default:
         return 0;
    }
}

uint32_t
compute_image_group_size (int format,
                          int type)
{
  return bytes_per_element (type) * elements_per_group (format, type);
}

size_t
_get_egl_attrib_list_size (const EGLint *attrib_list)
{
    size_t offset = 0;
    while (attrib_list[offset] != EGL_NONE)
        offset += 2;
    return offset + 1;
}

