#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""code generator for GLES2 command buffers."""

import os
import os.path
import sys
import re
from optparse import OptionParser

_SIZE_OF_UINT32 = 4
_SIZE_OF_COMMAND_HEADER = 4
_FIRST_SPECIFIC_COMMAND_ID = 256

_DO_NOT_EDIT_WARNING = """// This file is auto-generated. DO NOT EDIT!

"""

_DEFAULT_RETURN_VALUES = {
    '__eglMustCastToProperFunctionPointerType': 'NULL',
    'const char*': 'NULL',
    'const GLubyte*': 'NULL',
    'void*': 'NULL',
    'GLuint': '0',
    'GLint': '0',
    'GLenum': 'GL_INVALID_ENUM',
    'GLboolean': 'GL_FALSE',
    'EGLDisplay': 'EGL_NO_DISPLAY',
    'EGLBoolean': 'EGL_FALSE',
    'EGLSurface': 'EGL_NO_SURFACE',
    'EGLenum': 'EGL_NONE',
    'EGLContext': 'EGL_NO_CONTEXT',
    'EGLImageKHR': 'EGL_NO_IMAGE_KHR',
    'EGLSyncKHR': 'EGL_NO_SYNC_KHR',
    'EGLSyncNV': 'EGL_NO_SYNC_NV'
}

# This table specifies types and other special data for the commands that
# will be generated.
#
# Must match function names specified in "cmd_buffer_functions.txt".
#
# type:             Defines how this method is generated.

#                   Manual: The entry point into the client is custom,
#                   but the command struct and command init function are not.
#
#                   Passthrough: Arguments that are pointers are passed to
#                   the server thread without making a copy.

#                   Synchronous: The client will wait until the server finishes
#                   executing this command. Note that this implies "Passthrough."
#                   For the moment, all functions that return values or have out
#                   parameters are synchronous.

# default_return:   Defines what the default return value of this function is, if
#                   it differs from the list of default return values above.

_FUNCTION_INFO = {
  'glDrawArrays' : {
    'type': 'Synchronous',
  },
  'glDrawElements' : {
    'type': 'Synchronous',
  },
  'glMultiDrawArraysEXT' : {
    'type': 'Synchronous',
  },
  'glMultiDrawElementsEXT' : {
    'type': 'Synchronous',
  },
  'glTexImage3DOES' : {
    'type': 'Synchronous',
  },
  'glTexSubImage3DOES' : {
    'type': 'Synchronous',
  },
  'eglGetProcAddress': {
    'type': 'Manual',
  },
  'glFlush' : {
    'type': 'Synchronous',
  },
  'glFinish' : {
    'type': 'Synchronous',
  },
  # This pointer should be valid until glDrawElements or glDrawArray is
  # called so we can just pass them through without copying. A more advanced
  # implementation would wait until glDrawElements/glDrawArray is called
  # and then make a copy.
  'glVertexAttribPointer': {
    'type': 'Passthrough',
  },
  'eglGetError': {
    'default_return': 'EGL_NOT_INITIALIZED'
  },
  'eglWaitGL': {
    'default_return': 'EGL_TRUE'
  },
  'eglClientWaitSyncKHR': {
    'default_return': 'EGL_FALSE'
  },
  'eglClientWaitSyncNV': {
    'default_return': 'EGL_TIMEOUT_EXPIRED_NV'
  },
  'glGetUniformLocation': {
    'default_return': '-1',
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['program']}
  },
  'glGetAttribLocation': {
    'default_return': '-1',
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['program']}
  },
  'glGetError': {
    'default_return': 'GL_INVALID_OPERATION',
  },
  'glBufferData': {
    'argument_has_size': { 'data': 'size' }
  },
  'glBufferSubData': {
    'argument_has_size': { 'data': 'size' }
  },
  'glCompressedTexImage2D': {
    'argument_has_size': { 'data': 'imageSize' }
  },
  'glCompressedTexSubImage2D': {
    'argument_has_size': { 'data': 'imageSize' }
  },
  'glCompressedTexImage3DOES': {
    'argument_has_size': { 'data': 'imageSize' }
  },
  'glCompressedTexSubImage3DOES': {
    'argument_has_size': { 'data': 'imageSize' }
  },
  'glCompressedTexSubImage2D': {
    'argument_has_size': { 'data': 'imageSize' }
  },
  'glDeleteBuffers': {
    'argument_has_size': { 'buffers': 'n' }
  },
  'glDeleteBuffers': {
    'argument_has_size': { 'buffers': 'n' }
  },
  'glDeleteFramebuffers': {
    'argument_has_size': { 'framebuffers': 'n' }
  },
  'glDeleteRenderbuffers': {
    'argument_has_size': { 'renderbuffers': 'n' }
  },
  'glDeleteTextures': {
    'argument_has_size': { 'textures': 'n' }
  },
  'glDeleteVertexArraysOES': {
    'argument_has_size': { 'arrays': 'n' }
  },
  'glDeleteVertexArraysOES': {
    'argument_has_size': { 'arrays': 'n' }
  },
  'glDiscardFramebufferEXT': {
    'argument_has_size': { 'attachments': 'numAttachments' }
  },
  'glDeleteFencesNV': {
    'argument_has_size': { 'fences': 'count' }
  },
  'glDeleteQueriesEXT': {
    'argument_has_size': { 'queries': 'n' }
  },
  'glShaderBinary': {
    'argument_has_size': { 'shaders': 'n', 'binary': 'length' }
  },
  'glUniform1fv': {
    'argument_has_size': { 'v': 'count' },
    'argument_element_size': { 'v': 1 }
  },
  'glUniform2fv': {
    'argument_has_size': { 'v': 'count' },
    'argument_element_size': { 'v': 2 }
  },
  'glUniform3fv': {
    'argument_has_size': { 'v': 'count' },
    'argument_element_size': { 'v': 3 }
  },
  'glUniform4fv': {
    'argument_has_size': { 'v': 'count' },
    'argument_element_size': { 'v': 4 }
  },
  'glUniform1iv': {
    'argument_has_size': { 'v': 'count' },
    'argument_element_size': { 'v': 1 }
  },
  'glUniform2iv': {
    'argument_has_size': { 'v': 'count' },
    'argument_element_size': { 'v': 2 }
  },
  'glUniform3iv': {
    'argument_has_size': { 'v': 'count' },
    'argument_element_size': { 'v': 3 }
  },
  'glUniform4iv': {
    'argument_has_size': { 'v': 'count' },
    'argument_element_size': { 'v': 4 }
  },
  'glUniformMatrix2fv': {
    'argument_has_size': { 'value': 'count' },
    'argument_element_size': { 'value': 4 }
  },
  'glUniformMatrix3fv': {
    'argument_has_size': { 'value': 'count' },
    'argument_element_size': { 'value': 9 }
  },
  'glUniformMatrix4fv': {
    'argument_has_size': { 'value': 'count' },
    'argument_element_size': { 'value': 16 }
  },
  'glVertexAttrib1fv': {
    'argument_element_size': { 'values': 1 }
  },
  'glVertexAttrib2fv': {
    'argument_element_size': { 'values': 2 }
  },
  'glVertexAttrib3fv': {
    'argument_element_size': { 'values': 3 }
  },
  'glVertexAttrib4fv': {
    'argument_element_size': { 'values': 4 }
  },
  'glProgramBinaryOES': {
    'argument_has_size': { 'binary': 'length' }
  },
  'glDeletePerfMonitorsAMD': {
    'argument_has_size': { 'monitors': 'n' }
  },
  'glSelectPerfMonitorCountersAMD': {
    'argument_has_size': { 'countersList': 'numCounters' }
  },
  'eglInitialize': {
    'out_arguments': ['major', 'minor']
  },
  'eglGetConfigs': {
    'out_arguments': ['configs', 'num_config']
  },
  'eglGetConfigAttrib': {
    'out_arguments': ['value']
  },
  'eglQuerySurface': {
    'out_arguments': ['value']
  },
  'eglQueryContext': {
    'out_arguments': ['value']
  },
  'eglGetSyncAttribKHR': {
    'out_arguments': ['value']
  },
  'eglGetSyncAttribNV': {
    'out_arguments': ['value']
  },
  'eglGetImageAttribSEC': {
    'out_arguments': ['value']
  },
  'eglQuerySurfacePointerANGLE': {
    'out_arguments': ['value']
  },
  'glGetBooleanv': {
    'out_arguments': ['params']
  },
  'glGetFloatv': {
    'out_arguments': ['params']
  },
  'glGetIntegerv': {
    'out_arguments': ['params']
  },
  'glGenBuffers': {
    'type': 'Asynchronous',
    'out_arguments': ['buffers']
  },
  'glGenFramebuffers': {
    'type': 'Asynchronous',
    'out_arguments': ['framebuffers']
  },
  'glGenRenderbuffers': {
    'type': 'Asynchronous',
    'out_arguments': ['renderbuffers']
  },
  'glGenTextures': {
    'type': 'Asynchronous',
    'out_arguments': ['textures']
  },
  'glCreateProgram': {
    'type': 'Asynchronous'
  },
  'glCreateShader': {
    'type': 'Asynchronous'
  },
  'glGetActiveAttrib': {
    'out_arguments': ['length', 'size', 'type', 'name'],
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['program']}
  },
  'glGetActiveUniform': {
    'out_arguments': ['length', 'size', 'type', 'name'],
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['program']}
  },
  'glGetAttachedShaders': {
    'out_arguments': ['count', 'shaders'],
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['program']}
  },
  'glGetBufferParameteriv': {
    'out_arguments': {'attrib_list': ['params']}
  },
  'glGetFramebufferAttachmentParameteriv': {
    'out_arguments': {'attrib_list': ['params']}
  },
  'glGetProgramInfoLog': {
    'out_arguments': ['length', 'infolog'],
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['program']}
  },
  'glGetProgramiv': {
    'out_arguments': ['params'],
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['program']}
  },
  'glGetRenderbufferParameteriv': {
    'out_arguments': ['params']
  },
  'glGetShaderInfoLog': {
    'out_arguments': ['length', 'infolog'],
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['shader']}
  },
  'glGetShaderPrecisionFormat': {
    'out_arguments': ['range', 'precision']
  },
  'glGetShaderSource': {
    'out_arguments': ['length', 'source'],
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['shader']}
  },
  'glGetShaderiv': {
    'out_arguments': ['params'],
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['shader']}
  },
  'glGetTexParameteriv': {
    'out_arguments': ['params']
  },
  'glGetTexParameterfv': {
    'out_arguments': ['params']
  },
  'glGetUniformiv': {
    'out_arguments': ['params'],
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['program']}
  },
  'glGetUniformfv': {
    'out_arguments': ['params'],
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['program']}
  },
  'glGetVertexAttribiv': {
    'out_arguments': ['params']
  },
  'glGetVertexAttribfv': {
    'out_arguments': ['params']
  },
  'glReadPixels': {
    'out_arguments': ['pixels']
  },
  'glGetVertexAttribPointerv': {
    'out_arguments': ['pointer']
  },
  'glGetProgramBinaryOES': {
    'out_arguments': ['length', 'binaryFormat', 'binary']
  },
  'glGetBufferPointervOES': {
    'out_arguments': ['params']
  },
  'glGenVertexArraysOES': {
    'out_arguments': ['arrays']
  },
  'glGenPerfMonitorsAMD': {
    'out_arguments': ['monitors']
  },
  'glGetPerfMonitorGroupsAMD': {
    'out_arguments': ['numGroups', 'groups']
  },
  'glGetPerfMonitorCountersAMD': {
    'out_arguments': ['numCounters', 'maxActiveCounters', 'counters']
  },
  'glGetPerfMonitorCounterDataAMD': {
    'out_arguments': ['data', 'bytesWritten']
  },
  'glGetPerfMonitorGroupStringAMD': {
    'out_arguments': ['length', 'groupString']
  },
  'glGetPerfMonitorCounterStringAMD': {
    'out_arguments': ['length', 'counterString']
  },
  'glGetPerfMonitorCounterInfoAMD': {
    'out_arguments': ['data']
  },
  'glGenFencesNV': {
    'out_arguments': ['fences']
  },
  'glGetFenceivNV': {
    'out_arguments': ['params']
  },
  'glGetDriverControlsQCOM': {
    'out_arguments': ['num', 'driverControls']
  },
  'glGetDriverControlStringQCOM': {
    'out_arguments': ['length', 'driverControlString']
  },
  'glExtGetTexturesQCOM': {
    'out_arguments': ['textures', 'numTextures']
  },
  'glExtGetBuffersQCOM': {
    'out_arguments': ['buffers', 'numBuffers']
  },
  'glExtGetRenderbuffersQCOM': {
    'out_arguments': ['renderbuffers', 'numRenderbuffers']
  },
  'glExtGetFramebuffersQCOM': {
    'out_arguments': ['framebuffers', 'numFramebuffers']
  },
  'glExtGetTexLevelParameterivQCOM': {
    'out_arguments': ['params']
  },
  'glExtGetTexSubImageQCOM': {
    'out_arguments': ['texels']
  },
  'glExtGetBufferPointervQCOM': {
    'out_arguments': ['params']
  },
  'glExtGetShadersQCOM': {
    'out_arguments': ['shaders', 'numShaders']
  },
  'glExtGetProgramsQCOM': {
    'out_arguments': ['programs', 'numPrograms']
  },
  'glExtGetProgramBinarySourceQCOM': {
    'out_arguments': ['source', 'length']
  },
  'glGenQueriesEXT': {
    'out_arguments': ['queries']
  },
  'glGetQueryivEXT': {
    'out_arguments': ['params']
  },
  'glGetQueryObjectuivEXT': {
    'out_arguments': ['params']
  },
  'eglChooseConfig': {
    'out_arguments': ['configs', 'num_config'],
    'argument_size_from_function': {'attrib_list': '_get_egl_attrib_list_size'}
  },
  'eglCreateWindowSurface': {
    'argument_size_from_function': {'attrib_list': '_get_egl_attrib_list_size'}
  },
  'eglCreatePbufferSurface': {
    'argument_size_from_function': {'attrib_list': '_get_egl_attrib_list_size'}
  },
  'eglCreatePixmapSurface': {
    'argument_size_from_function': {'attrib_list': '_get_egl_attrib_list_size'}
  },
  'eglCreatePbufferFromClientBuffer': {
    'argument_size_from_function': {'attrib_list': '_get_egl_attrib_list_size'}
  },
  'eglCreateContext': {
    'argument_size_from_function': {'attrib_list': '_get_egl_attrib_list_size'}
  },
  'eglLockSurfaceKHR': {
    'argument_size_from_function': {'attrib_list': '_get_egl_attrib_list_size'}
  },
  'eglCreateImageKHR': {
    'argument_size_from_function': {'attrib_list': '_get_egl_attrib_list_size'}
  },
  'eglCreateSyncKHR': {
    'argument_size_from_function': {'attrib_list': '_get_egl_attrib_list_size'}
  },
  'eglCreateFenceSyncNV': {
    'argument_size_from_function': {'attrib_list': '_get_egl_attrib_list_size'}
  },
  'eglCreateDRMImageMESA': {
    'argument_size_from_function': {'attrib_list': '_get_egl_attrib_list_size'}
  },
  'glBindBuffer' : {
    'mapped_names': {'type': 'buffer','attrib_list': ['buffer'], 'create_attrib_if_needed': ['buffer']}
  },
  'glIsBuffer' : {
    'mapped_names': {'type': 'buffer', 'attrib_list': ['buffer']}
  },
  'glBindFramebuffer' : {
    'mapped_names': {'type': 'framebuffer', 'attrib_list': ['framebuffer']}
  },
  'glIsFramebuffer' : {
    'mapped_names': {'type': 'framebuffer', 'attrib_list': ['framebuffer']}
  },
  'glBindTexture' : {
    'mapped_names': {'type': 'texture', 'attrib_list': ['texture'], 'create_attrib_if_needed': ['texture']}
  },
  'glIsTexture' : {
    'mapped_names': {'type': 'texture', 'attrib_list': ['texture']}
  },
  'glFramebufferTexture2D' : {
    'mapped_names': {'type': 'texture', 'attrib_list': ['texture'], 'create_attrib_if_needed': ['texture']}
  },
  'glBindRenderbuffer' : {
    'mapped_names': {'type': 'renderbuffer', 'attrib_list': ['renderbuffer'], 'create_attrib_if_needed': ['renderbuffer']}
  },
  'glIsRenderbuffer' : {
    'mapped_names': {'type': 'renderbuffer', 'attrib_list': ['renderbuffer']}
  },
  'glFramebufferRenderbuffer' : {
    'mapped_names': {'type': 'renderbuffer', 'attrib_list': ['renderbuffer']}
  },
  'glFramebufferTexture2DMultisampleEXT': {
    'mapped_names': {'type': 'texture', 'attrib_list': ['texture']}
  },
  'glFramebufferTexture2DMultisampleIMG': {
    'mapped_names': {'type': 'texture', 'attrib_list': ['texture']}
  },
  'glAttachShader' : {
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['program', 'shader']}
  },
  'glBindAttribLocation' : {
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['program']}
  },
  'glDetachShader' : {
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['program', 'shader']}
  },
  'glIsProgram' : {
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['program']}
  },
  'glLinkProgram' : {
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['program']}
  },
  'glUseProgram' : {
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['program']}
  },
  'glValidateProgram' : {
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['program']}
  },
  'glCompileShader' : {
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['shader']}
  },
  'glDeleteShader' : {
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['shader']}
  },
  'glIsShader' : {
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['shader']}
  },
  'glShaderSource' : {
    'mapped_names': {'type': 'shader_object', 'attrib_list': ['shader']}
  },
}

FUNCTIONS_GENERATING_ERRORS = [
 'glAttachShader',
 'glBindAttribLocation',
 'glBindBuffer',
 'glBufferData',
 'glBufferSubData',
 'glCompileShader',
 'glCompressedTexImage2D',
 'glCompressedTexSubImage2D',
 'glCopyTexImage2D',
 'glCopyTexSubImage2D',
 'glDeleteProgram',
 'glDeleteShader',
 'glDetachShader',
 'glFramebufferRenderbuffer',
 'glFramebufferTexture2D',
 'glGenerateMipmap',
 'glGetBufferParameteriv',
 'glGetIntegerv',
 'glGetProgramInfoLog',
 'glGetShaderInfoLog',
 'glGetShaderiv',
 'glGetShaderPrecisionFormat',
 'glGetShaderSource',
 'glLinkProgram',
 'glReadPixels',
 'glReleaseShaderCompiler',
 'glRenderbufferStorage',
 'glShaderBinary',
 'glValidateProgram',
 'glVertexAttribPointer',
 'glBindVertexArrayOES',
 'glEGLImageTargetTexture2DOES',
 'glGetProgramBinaryOES',
 'glProgramBinaryOES',
 'glGetBufferPointervOES',
 'glTexImage3DOES',
 'glTexSubImage3DOES',
 'glCopyTexSubImage3DOES',
 'glCompressedTexImage3DOES',
 'glCompressedTexSubImage3DOES',
 'glFramebufferTexture2DMultisampleEXT',
 'glFramebufferTexture2DMultisampleIMG',
 'glFramebufferTexture3DOES',
 'glBeginPerfMonitorAMD',
 'glGetPerfMonitorGroupsAMD',
 'glGetPerfMonitorCountersAMD',
 'glGetPerfMonitorGroupStringAMD',
 'glGetPerfMonitorCounterStringAMD',
 'glGetPerfMonitorCounterInfoAMD',
 'glGenPerfMonitorsAMD',
 'glEndPerfMonitorAMD',
 'glDeletePerfMonitorsAMD',
 'glSelectPerfMonitorCountersAMD',
 'glGetPerfMonitorCounterDataAMD',
 'glBlitFramebufferANGLE',
 'glRenderbufferStorageMultisampleANGLE',
 'glRenderbufferStorageMultisampleAPPLE',
 'glResolveMultisampleFramebufferAPPLE',
 'glRenderbufferStorageMultisampleEXT',
 'glRenderbufferStorageMultisampleIMG',
 'glSetFenceNV',
 'glFinishFenceNV',
 'glCoverageMaskNV',
 'glGetDriverControlsQCOM',
 'glEnableDriverControlQCOM',
 'glDisableDriverControlQCOM',
 'glExtTexObjectStateOverrideiQCOM',
 'glGetDriverControlStringQCOM',
 'glExtGetTexLevelParameterivQCOM',
 'glExtGetTexSubImageQCOM',
 'glExtGetBufferPointervQCOM',
 'glExtIsProgramBinaryQCOM',
 'glExtGetProgramBinarySourceQCOM',
 'glStartTilingQCOM',
 'glEndTilingQCOM',
]

_GL_GET_TYPE_INFO_FUNC = {
  'glGetBooleanv': { 'type': 'GLboolean'},
  'glGetFloatv': { 'type': 'GLfloat'},
  'glGetIntegerv': { 'type': 'GLint'}
}

_GL_GET_TYPE_INFO = {
  'GL_BLEND': {
    'var':'blend'
  },
  'GL_COLOR_WRITEMASK': {
    'var': 'color_writemask',
    'size': 4
  },
  'GL_CULL_FACE': {
    'var': 'cull_face'
  },
  'GL_DEPTH_TEST': {
    'var': 'depth_test'
  },
  'GL_DEPTH_WRITEMASK': {
    'var': 'depth_writemask'
  },
  'GL_DITHER': {
    'var': 'dither'
  },
  'GL_POLYGON_OFFSET_FILL': {
    'var': 'polygon_offset_fill'
  },
  'GL_SAMPLE_ALPHA_TO_COVERAGE': {
    'var': 'sample_alpha_to_coverage'
  },
  'GL_SAMPLE_COVERAGE': {
    'var': 'sample_coverage'
  },
  'GL_SCISSOR_TEST': {
    'var': 'scissor_test'
  },
  'GL_SHADER_COMPILER': {
    'var': 'shader_compiler'
  },
  'GL_STENCIL_TEST': {
    'var': 'stencil_test'
  },
  'GL_BLEND_COLOR': {
    'var': 'blend_color',
    'size': 4
  },
  'GL_BLEND_DST_ALPHA': {
    'var': 'blend_dst',
    'index': 1
  },
  'GL_BLEND_DST_RGB': {
    'var': 'blend_dst',
    'index': 0
  },
  'GL_BLEND_EQUATION_ALPHA': {
    'var': 'blend_equation',
    'index': 1
  },
  'GL_BLEND_EQUATION_RGB': {
    'var': 'blend_equation',
    'index': 0
  },
  'GL_BLEND_SRC_ALPHA': {
    'var': 'blend_src',
    'index': 1
  },
  'GL_BLEND_SRC_RGB': {
    'var': 'blend_src',
    'index': 0
  },
  'GL_COLOR_CLEAR_VALUE': {
    'var': 'color_clear_value',
    'size': 4
  },
  'GL_DEPTH_CLEAR_VALUE': {
    'var': 'depth_clear_value',
  },
  'GL_DEPTH_RANGE': {
    'var': 'depth_range',
    'size': 2
  },
  'GL_LINE_WIDTH': {
    'var': 'line_width'
  },
  'GL_POLYGON_OFFSET_FACTOR': {
    'var': 'polygon_offset_factor'
  },
  'GL_ACTIVE_TEXTURE': {
    'var': 'active_texture'
  },
  'GL_CURRENT_PROGRAM': {
    'var': 'current_program'
  },
  'GL_DEPTH_CLEAR_VALUE': {
    'var': 'depth_clear_value'
  },
  'GL_DEPTH_FUNC': {
    'var': 'depth_func'
  },
  'GL_DEPTH_RANGE': {
    'var': 'depth_range',
    'size': 2
  },
  'GL_GENERATE_MIPMAP_HINT': {
    'var': 'generate_mipmap_hint'
  },
  'GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS': {
    'var': 'max_combined_texture_image_units',
    'has_cache': True
  },
  'GL_MAX_CUBE_MAP_TEXTURE_SIZE': {
    'var': 'max_cube_map_texture_size',
    'has_cache': True
  },
  'GL_MAX_FRAGMENT_UNIFORM_VECTORS': {
    'var': 'max_fragment_uniform_vectors',
    'has_cache': True
  },
  'GL_MAX_RENDERBUFFER_SIZE': {
    'var': 'max_renderbuffer_size',
    'has_cache': True
  },
  'GL_MAX_TEXTURE_IMAGE_UNITS': {
    'var': 'max_texture_image_units',
    'has_cache': True
  },
  'GL_MAX_VARYING_VECTORS': {
    'var': 'max_varying_vectors',
    'has_cache': True
  },
  'GL_MAX_TEXTURE_SIZE': {
    'var': 'max_texture_size',
    'has_cache': True
  },
  'GL_MAX_VERTEX_ATTRIBS': {
    'var': 'max_vertex_attribs',
    'has_cache': True
  },
  'GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS': {
    'var': 'max_vertex_texture_image_units',
    'has_cache': True
  },
  'GL_MAX_VERTEX_UNIFORM_VECTORS': {
    'var': 'max_vertex_uniform_vectors',
    'has_cache': True
  },
  'GL_POLYGON_OFFSET_UNITS': {
    'var': 'polygon_offset_units'
  },
  'GL_SCISSOR_BOX': {
    'var': 'scissor_box',
    'size': 4
  },
  'GL_STENCIL_BACK_FAIL': {
    'var': 'stencil_back_fail'
  },
  'GL_STENCIL_BACK_FUNC': {
    'var': 'stencil_back_func'
  },
  'GL_STENCIL_BACK_PASS_DEPTH_FAIL': {
    'var': 'stencil_back_pass_depth_fail'
  },
  'GL_STENCIL_BACK_PASS_DEPTH_PASS': {
    'var': 'stencil_back_pass_depth_pass'
  },
  'GL_STENCIL_BACK_REF': {
    'var': 'stencil_ref'
  },
  'GL_STENCIL_BACK_VALUE_MASK': {
    'var': 'stencil_value_mask'
  },
  'GL_STENCIL_CLEAR_VALUE': {
    'var': 'stencil_clear_value'
  },
  'GL_STENCIL_FAIL': {
    'var': 'stencil_fail'
  },
  'GL_STENCIL_FUNC': {
    'var': 'stencil_func'
  },
  'GL_STENCIL_PASS_DEPTH_FAIL': {
    'var': 'stencil_pass_depth_fail'
  },
  'GL_STENCIL_PASS_DEPTH_PASS': {
    'var': 'stencil_pass_depth_pass'
  },
  'GL_STENCIL_REF': {
    'var': 'stencil_ref'
  },
  'GL_STENCIL_VALUE_MASK': {
    'var': 'stencil_value_mask'
  },
  'GL_STENCIL_WRITEMASK': {
    'var': 'stencil_writemask'
  },
  'GL_STENCIL_BACK_WRITEMASK': {
    'var': 'stencil_back_writemask'
  },
  'GL_VIEWPORT': {
    'var': 'viewport',
    'size': 4
  },
  'GL_ARRAY_BUFFER_BINDING': {
    'var': 'array_buffer_binding',
    'fetch_server_data': True
  },
  'GL_ELEMENT_ARRAY_BUFFER_BINDING': {
    'var': 'array_buffer_binding',
    'fetch_server_data': True
  },
  'GL_VERTEX_ARRAY_BINDING_OES': {
    'var': 'vertex_array_binding',
    'fetch_server_data': True
  },
  'GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT': {
    'var': 'max_texture_max_anisotropy',
    'has_cache': True
  },
  'GL_UNPACK_ROW_LENGTH': {
    'var': 'unpack_row_length'
  },
  'GL_UNPACK_SKIP_ROWS': {
    'var': 'unpack_skip_rows'
  },
  'GL_UNPACK_SKIP_PIXELS': {
    'var': 'unpack_skip_pixels'
  }
}

# This string is copied directly out of the gl2.h file from GLES2.0
#
# Edits:
#
# *) Any argument that is a resourceID has been changed to GLid<Type>.
#    (not pointer arguments) and if it's allowed to be zero it's GLidZero<Type>
#    If it's allowed to not exist it's GLidBind<Type>
#
# *) All GLenums have been changed to GLenumTypeOfEnum
#
_GL_TYPES = {
  'GLenum': 'unsigned int',
  'GLboolean': 'unsigned char',
  'GLbitfield': 'unsigned int',
  'GLbyte': 'signed char',
  'GLshort': 'short',
  'GLint': 'int',
  'GLsizei': 'int',
  'GLubyte': 'unsigned char',
  'GLushort': 'unsigned short',
  'GLuint': 'unsigned int',
  'GLfloat': 'float',
  'GLclampf': 'float',
  'GLvoid': 'void',
  'GLfixed': 'int',
  'GLclampx': 'int',
  'GLintptr': 'long int',
  'GLsizeiptr': 'long int',
  'GLvoid': 'void',
  'GLeglImageOES' : 'void*'
}

# This is a list of enum names and their valid values. It is used to map
# GLenum arguments to a specific set of valid values.
_ENUM_LISTS = {
  'BlitFilter': {
    'type': 'GLenum',
    'valid': [
      'GL_NEAREST',
      'GL_LINEAR',
    ],
    'invalid': [
      'GL_LINEAR_MIPMAP_LINEAR',
    ],
  },
  'FrameBufferTarget': {
    'type': 'GLenum',
    'valid': [
      'GL_FRAMEBUFFER',
    ],
    'invalid': [
      'GL_DRAW_FRAMEBUFFER' ,
      'GL_READ_FRAMEBUFFER' ,
    ],
  },
  'RenderBufferTarget': {
    'type': 'GLenum',
    'valid': [
      'GL_RENDERBUFFER',
    ],
    'invalid': [
      'GL_FRAMEBUFFER',
    ],
  },
  'BufferTarget': {
    'type': 'GLenum',
    'valid': [
      'GL_ARRAY_BUFFER',
      'GL_ELEMENT_ARRAY_BUFFER',
    ],
    'invalid': [
      'GL_RENDERBUFFER',
    ],
  },
  'BufferUsage': {
    'type': 'GLenum',
    'valid': [
      'GL_STREAM_DRAW',
      'GL_STATIC_DRAW',
      'GL_DYNAMIC_DRAW',
    ],
    'invalid': [
      'GL_STATIC_READ',
    ],
  },
  'GLState': {
    'type': 'GLenum',
    'valid': [
      'GL_ACTIVE_TEXTURE',
      'GL_ALIASED_LINE_WIDTH_RANGE',
      'GL_ALIASED_POINT_SIZE_RANGE',
      'GL_ALPHA_BITS',
      'GL_ARRAY_BUFFER_BINDING',
      'GL_BLEND',
      'GL_BLEND_COLOR',
      'GL_BLEND_DST_ALPHA',
      'GL_BLEND_DST_RGB',
      'GL_BLEND_EQUATION_ALPHA',
      'GL_BLEND_EQUATION_RGB',
      'GL_BLEND_SRC_ALPHA',
      'GL_BLEND_SRC_RGB',
      'GL_BLUE_BITS',
      'GL_COLOR_CLEAR_VALUE',
      'GL_COLOR_WRITEMASK',
      'GL_COMPRESSED_TEXTURE_FORMATS',
      'GL_CULL_FACE',
      'GL_CULL_FACE_MODE',
      'GL_CURRENT_PROGRAM',
      'GL_DEPTH_BITS',
      'GL_DEPTH_CLEAR_VALUE',
      'GL_DEPTH_FUNC',
      'GL_DEPTH_RANGE',
      'GL_DEPTH_TEST',
      'GL_DEPTH_WRITEMASK',
      'GL_DITHER',
      'GL_ELEMENT_ARRAY_BUFFER_BINDING',
      'GL_FRAMEBUFFER_BINDING',
      'GL_FRONT_FACE',
      'GL_GENERATE_MIPMAP_HINT',
      'GL_GREEN_BITS',
      'GL_IMPLEMENTATION_COLOR_READ_FORMAT',
      'GL_IMPLEMENTATION_COLOR_READ_TYPE',
      'GL_LINE_WIDTH',
      'GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS',
      'GL_MAX_CUBE_MAP_TEXTURE_SIZE',
      'GL_MAX_FRAGMENT_UNIFORM_VECTORS',
      'GL_MAX_RENDERBUFFER_SIZE',
      'GL_MAX_TEXTURE_IMAGE_UNITS',
      'GL_MAX_TEXTURE_SIZE',
      'GL_MAX_VARYING_VECTORS',
      'GL_MAX_VERTEX_ATTRIBS',
      'GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS',
      'GL_MAX_VERTEX_UNIFORM_VECTORS',
      'GL_MAX_VIEWPORT_DIMS',
      'GL_NUM_COMPRESSED_TEXTURE_FORMATS',
      'GL_NUM_SHADER_BINARY_FORMATS',
      'GL_PACK_ALIGNMENT',
      'GL_POLYGON_OFFSET_FACTOR',
      'GL_POLYGON_OFFSET_FILL',
      'GL_POLYGON_OFFSET_UNITS',
      'GL_RED_BITS',
      'GL_RENDERBUFFER_BINDING',
      'GL_SAMPLE_BUFFERS',
      'GL_SAMPLE_COVERAGE_INVERT',
      'GL_SAMPLE_COVERAGE_VALUE',
      'GL_SAMPLES',
      'GL_SCISSOR_BOX',
      'GL_SCISSOR_TEST',
      'GL_SHADER_BINARY_FORMATS',
      'GL_SHADER_COMPILER',
      'GL_STENCIL_BACK_FAIL',
      'GL_STENCIL_BACK_FUNC',
      'GL_STENCIL_BACK_PASS_DEPTH_FAIL',
      'GL_STENCIL_BACK_PASS_DEPTH_PASS',
      'GL_STENCIL_BACK_REF',
      'GL_STENCIL_BACK_VALUE_MASK',
      'GL_STENCIL_BACK_WRITEMASK',
      'GL_STENCIL_BITS',
      'GL_STENCIL_CLEAR_VALUE',
      'GL_STENCIL_FAIL',
      'GL_STENCIL_FUNC',
      'GL_STENCIL_PASS_DEPTH_FAIL',
      'GL_STENCIL_PASS_DEPTH_PASS',
      'GL_STENCIL_REF',
      'GL_STENCIL_TEST',
      'GL_STENCIL_VALUE_MASK',
      'GL_STENCIL_WRITEMASK',
      'GL_SUBPIXEL_BITS',
      'GL_TEXTURE_BINDING_2D',
      'GL_TEXTURE_BINDING_CUBE_MAP',
      'GL_UNPACK_ALIGNMENT',
      'GL_VIEWPORT',
    ],
    'invalid': [
      'GL_FOG_HINT',
    ],
  },
  'GetTexParamTarget': {
    'type': 'GLenum',
    'valid': [
      'GL_TEXTURE_2D',
      'GL_TEXTURE_CUBE_MAP',
      'GL_TEXTURE_3D_OES',
    ],
    'invalid': [
      'GL_PROXY_TEXTURE_CUBE_MAP',
    ]
  },
  'TextureTarget': {
    'type': 'GLenum',
    'valid': [
      'GL_TEXTURE_2D',
      'GL_TEXTURE_CUBE_MAP_POSITIVE_X',
      'GL_TEXTURE_CUBE_MAP_NEGATIVE_X',
      'GL_TEXTURE_CUBE_MAP_POSITIVE_Y',
      'GL_TEXTURE_CUBE_MAP_NEGATIVE_Y',
      'GL_TEXTURE_CUBE_MAP_POSITIVE_Z',
      'GL_TEXTURE_CUBE_MAP_NEGATIVE_Z',
    ],
    'invalid': [
      'GL_PROXY_TEXTURE_CUBE_MAP',
    ]
  },
  'TextureBindTarget': {
    'type': 'GLenum',
    'valid': [
      'GL_TEXTURE_2D',
      'GL_TEXTURE_CUBE_MAP',
      'GL_TEXTURE_3D_OES',
    ],
    'invalid': [
      'GL_TEXTURE_1D',
      'GL_TEXTURE_3D',
    ],
  },
  'ShaderType': {
    'type': 'GLenum',
    'valid': [
      'GL_VERTEX_SHADER',
      'GL_FRAGMENT_SHADER',
    ],
    'invalid': [
      'GL_GEOMETRY_SHADER',
    ],
  },
  'FaceType': {
    'type': 'GLenum',
    'valid': [
      'GL_FRONT',
      'GL_BACK',
      'GL_FRONT_AND_BACK',
    ],
  },
  'FaceMode': {
    'type': 'GLenum',
    'valid': [
      'GL_CW',
      'GL_CCW',
    ],
  },
  'CmpFunction': {
    'type': 'GLenum',
    'valid': [
      'GL_NEVER',
      'GL_LESS',
      'GL_EQUAL',
      'GL_LEQUAL',
      'GL_GREATER',
      'GL_NOTEQUAL',
      'GL_GEQUAL',
      'GL_ALWAYS',
    ],
  },
  'Equation': {
    'type': 'GLenum',
    'valid': [
      'GL_FUNC_ADD',
      'GL_FUNC_SUBTRACT',
      'GL_FUNC_REVERSE_SUBTRACT',
    ],
    'invalid': [
      'GL_MIN',
      'GL_MAX',
    ],
  },
  'SrcBlendFactor': {
    'type': 'GLenum',
    'valid': [
      'GL_ZERO',
      'GL_ONE',
      'GL_SRC_COLOR',
      'GL_ONE_MINUS_SRC_COLOR',
      'GL_DST_COLOR',
      'GL_ONE_MINUS_DST_COLOR',
      'GL_SRC_ALPHA',
      'GL_ONE_MINUS_SRC_ALPHA',
      'GL_DST_ALPHA',
      'GL_ONE_MINUS_DST_ALPHA',
      'GL_CONSTANT_COLOR',
      'GL_ONE_MINUS_CONSTANT_COLOR',
      'GL_CONSTANT_ALPHA',
      'GL_ONE_MINUS_CONSTANT_ALPHA',
      'GL_SRC_ALPHA_SATURATE',
    ],
  },
  'DstBlendFactor': {
    'type': 'GLenum',
    'valid': [
      'GL_ZERO',
      'GL_ONE',
      'GL_SRC_COLOR',
      'GL_ONE_MINUS_SRC_COLOR',
      'GL_DST_COLOR',
      'GL_ONE_MINUS_DST_COLOR',
      'GL_SRC_ALPHA',
      'GL_ONE_MINUS_SRC_ALPHA',
      'GL_DST_ALPHA',
      'GL_ONE_MINUS_DST_ALPHA',
      'GL_CONSTANT_COLOR',
      'GL_ONE_MINUS_CONSTANT_COLOR',
      'GL_CONSTANT_ALPHA',
      'GL_ONE_MINUS_CONSTANT_ALPHA',
    ],
  },
  'Capability': {
    'type': 'GLenum',
    'valid': [
      'GL_DITHER',  # 1st one is a non-cached value so autogen unit tests work.
      'GL_BLEND',
      'GL_CULL_FACE',
      'GL_DEPTH_TEST',
      'GL_POLYGON_OFFSET_FILL',
      'GL_SAMPLE_ALPHA_TO_COVERAGE',
      'GL_SAMPLE_COVERAGE',
      'GL_SCISSOR_TEST',
      'GL_STENCIL_TEST',
    ],
    'invalid': [
      'GL_CLIP_PLANE0',
      'GL_POINT_SPRITE',
    ],
  },
  'DrawMode': {
    'type': 'GLenum',
    'valid': [
      'GL_POINTS',
      'GL_LINE_STRIP',
      'GL_LINE_LOOP',
      'GL_LINES',
      'GL_TRIANGLE_STRIP',
      'GL_TRIANGLE_FAN',
      'GL_TRIANGLES',
    ],
    'invalid': [
      'GL_QUADS',
      'GL_POLYGON',
    ],
  },
  'IndexType': {
    'type': 'GLenum',
    'valid': [
      'GL_UNSIGNED_BYTE',
      'GL_UNSIGNED_SHORT',
    ],
    'invalid': [
      'GL_UNSIGNED_INT',
      'GL_INT',
    ],
  },
  'GetMaxIndexType': {
    'type': 'GLenum',
    'valid': [
      'GL_UNSIGNED_BYTE',
      'GL_UNSIGNED_SHORT',
      'GL_UNSIGNED_INT',
    ],
    'invalid': [
      'GL_INT',
    ],
  },
  'Attachment': {
    'type': 'GLenum',
    'valid': [
      'GL_COLOR_ATTACHMENT0',
      'GL_DEPTH_ATTACHMENT',
      'GL_STENCIL_ATTACHMENT',
       'GL_COLOR_EXT',
       'GL_DEPTH_EXT',
       'GL_STENCIL_EXT',
    ],
  },
  'BufferParameter': {
    'type': 'GLenum',
    'valid': [
      'GL_BUFFER_SIZE',
      'GL_BUFFER_USAGE',
    ],
    'invalid': [
      'GL_PIXEL_PACK_BUFFER',
    ],
  },
  'FrameBufferParameter': {
    'type': 'GLenum',
    'valid': [
      'GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE',
      'GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME',
      'GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL',
      'GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE',
    ],
  },
  'ProgramParameter': {
    'type': 'GLenum',
    'valid': [
      'GL_DELETE_STATUS',
      'GL_LINK_STATUS',
      'GL_VALIDATE_STATUS',
      'GL_INFO_LOG_LENGTH',
      'GL_ATTACHED_SHADERS',
      'GL_ACTIVE_ATTRIBUTES',
      'GL_ACTIVE_ATTRIBUTE_MAX_LENGTH',
      'GL_ACTIVE_UNIFORMS',
      'GL_ACTIVE_UNIFORM_MAX_LENGTH',
    ],
  },
  'QueryObjectParameter': {
    'type': 'GLenum',
    'valid': [
      'GL_QUERY_RESULT_EXT',
      'GL_QUERY_RESULT_AVAILABLE_EXT',
    ],
  },
  'QueryParameter': {
    'type': 'GLenum',
    'valid': [
      'GL_CURRENT_QUERY_EXT',
    ],
  },
  'QueryTarget': {
    'type': 'GLenum',
    'valid': [
      'GL_ANY_SAMPLES_PASSED_EXT',
      'GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT',
    ],
  },
  'RenderBufferParameter': {
    'type': 'GLenum',
    'valid': [
      'GL_RENDERBUFFER_RED_SIZE',
      'GL_RENDERBUFFER_GREEN_SIZE',
      'GL_RENDERBUFFER_BLUE_SIZE',
      'GL_RENDERBUFFER_ALPHA_SIZE',
      'GL_RENDERBUFFER_DEPTH_SIZE',
      'GL_RENDERBUFFER_STENCIL_SIZE',
      'GL_RENDERBUFFER_WIDTH',
      'GL_RENDERBUFFER_HEIGHT',
      'GL_RENDERBUFFER_INTERNAL_FORMAT',
    ],
  },
  'ShaderParameter': {
    'type': 'GLenum',
    'valid': [
      'GL_SHADER_TYPE',
      'GL_DELETE_STATUS',
      'GL_COMPILE_STATUS',
      'GL_INFO_LOG_LENGTH',
      'GL_SHADER_SOURCE_LENGTH',
      'GL_TRANSLATED_SHADER_SOURCE_LENGTH_ANGLE',
    ],
  },
  'ShaderPrecision': {
    'type': 'GLenum',
    'valid': [
      'GL_LOW_FLOAT',
      'GL_MEDIUM_FLOAT',
      'GL_HIGH_FLOAT',
      'GL_LOW_INT',
      'GL_MEDIUM_INT',
      'GL_HIGH_INT',
    ],
  },
  'StringType': {
    'type': 'GLenum',
    'valid': [
      'GL_VENDOR',
      'GL_RENDERER',
      'GL_VERSION',
      'GL_SHADING_LANGUAGE_VERSION',
      'GL_EXTENSIONS',
    ],
  },
  'TextureParameter': {
    'type': 'GLenum',
    'valid': [
      'GL_TEXTURE_MAG_FILTER',
      'GL_TEXTURE_MIN_FILTER',
      'GL_TEXTURE_WRAP_S',
      'GL_TEXTURE_WRAP_T',
      'GL_TEXTURE_WRAP_R_OES',
      'GL_TEXTURE_MAX_ANISOTROPY_EXT',
    ],
    'invalid': [
      'GL_GENERATE_MIPMAP',
    ],
  },
  'TextureWrapMode': {
    'type': 'GLenum',
    'valid': [
      'GL_CLAMP_TO_EDGE',
      'GL_MIRRORED_REPEAT',
      'GL_REPEAT',
    ],
  },
  'TextureMinFilterMode': {
    'type': 'GLenum',
    'valid': [
      'GL_NEAREST',
      'GL_LINEAR',
      'GL_NEAREST_MIPMAP_NEAREST',
      'GL_LINEAR_MIPMAP_NEAREST',
      'GL_NEAREST_MIPMAP_LINEAR',
      'GL_LINEAR_MIPMAP_LINEAR',
    ],
  },
  'TextureMagFilterMode': {
    'type': 'GLenum',
    'valid': [
      'GL_NEAREST',
      'GL_LINEAR',
    ],
  },
  'TextureUsage': {
    'type': 'GLenum',
    'valid': [
      'GL_NONE',
      'GL_FRAMEBUFFER_ATTACHMENT_ANGLE',
    ],
  },
  'VertexAttribute': {
    'type': 'GLenum',
    'valid': [
      # some enum that the decoder actually passes through to GL needs
      # to be the first listed here since it's used in unit tests.
      'GL_VERTEX_ATTRIB_ARRAY_NORMALIZED',
      'GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING',
      'GL_VERTEX_ATTRIB_ARRAY_ENABLED',
      'GL_VERTEX_ATTRIB_ARRAY_SIZE',
      'GL_VERTEX_ATTRIB_ARRAY_STRIDE',
      'GL_VERTEX_ATTRIB_ARRAY_TYPE',
      'GL_CURRENT_VERTEX_ATTRIB',
    ],
  },
  'VertexPointer': {
    'type': 'GLenum',
    'valid': [
      'GL_VERTEX_ATTRIB_ARRAY_POINTER',
    ],
  },
  'HintTarget': {
    'type': 'GLenum',
    'valid': [
      'GL_GENERATE_MIPMAP_HINT',
    ],
    'invalid': [
      'GL_PERSPECTIVE_CORRECTION_HINT',
    ],
  },
  'HintMode': {
    'type': 'GLenum',
    'valid': [
      'GL_FASTEST',
      'GL_NICEST',
      'GL_DONT_CARE',
    ],
  },
  'PixelStore': {
    'type': 'GLenum',
    'valid': [
      'GL_PACK_ALIGNMENT',
      'GL_UNPACK_ALIGNMENT',
      'GL_UNPACK_ROW_LENGTH',
      'GL_UNPACK_SKIP_ROWS',
      'GL_UNPACK_SKIP_PIXELS'
    ],
    'invalid': [
      'GL_PACK_SWAP_BYTES',
      'GL_UNPACK_SWAP_BYTES',
    ],
  },
  'PixelStoreAlignment': {
    'type': 'GLint',
    'valid': [
      '1',
      '2',
      '4',
      '8',
    ],
    'invalid': [
      '3',
      '9',
    ],
  },
  'ReadPixelFormat': {
    'type': 'GLenum',
    'valid': [
      'GL_ALPHA',
      'GL_RGB',
      'GL_RGBA',
    ],
  },
  'PixelType': {
    'type': 'GLenum',
    'valid': [
      'GL_UNSIGNED_BYTE',
      'GL_UNSIGNED_SHORT_5_6_5',
      'GL_UNSIGNED_SHORT_4_4_4_4',
      'GL_UNSIGNED_SHORT_5_5_5_1',
    ],
    'invalid': [
      'GL_SHORT',
      'GL_INT',
    ],
  },
  'ReadPixelType': {
    'type': 'GLenum',
    'valid': [
      'GL_UNSIGNED_BYTE',
      'GL_UNSIGNED_SHORT_5_6_5',
      'GL_UNSIGNED_SHORT_4_4_4_4',
      'GL_UNSIGNED_SHORT_5_5_5_1',
    ],
    'invalid': [
      'GL_SHORT',
      'GL_INT',
    ],
  },
  'RenderBufferFormat': {
    'type': 'GLenum',
    'valid': [
      'GL_RGBA4',
      'GL_RGB565',
      'GL_RGB5_A1',
      'GL_DEPTH_COMPONENT16',
      'GL_STENCIL_INDEX8',
    ],
  },
  'StencilOp': {
    'type': 'GLenum',
    'valid': [
      'GL_KEEP',
      'GL_ZERO',
      'GL_REPLACE',
      'GL_INCR',
      'GL_INCR_WRAP',
      'GL_DECR',
      'GL_DECR_WRAP',
      'GL_INVERT',
    ],
  },
  'TextureFormat': {
    'type': 'GLenum',
    'valid': [
      'GL_ALPHA',
      'GL_LUMINANCE',
      'GL_LUMINANCE_ALPHA',
      'GL_RGB',
      'GL_RGBA',
      'GL_BGRA_EXT',
    ],
    'invalid': [
      'GL_BGRA',
      'GL_BGR',
    ],
  },
  'TextureInternalFormat': {
    'type': 'GLenum',
    'valid': [
      'GL_ALPHA',
      'GL_LUMINANCE',
      'GL_LUMINANCE_ALPHA',
      'GL_RGB',
      'GL_RGBA',
    ],
    'invalid': [
      'GL_BGRA',
      'GL_BGR',
    ],
  },
  'TextureInternalFormatStorage': {
    'type': 'GLenum',
    'valid': [
      'GL_RGB565',
      'GL_RGBA4',
      'GL_RGB5_A1',
      'GL_ALPHA8_EXT',
      'GL_LUMINANCE8_EXT',
      'GL_LUMINANCE8_ALPHA8_EXT',
      'GL_RGB8_OES',
      'GL_RGBA8_OES',
    ],
  },
  'VertexAttribType': {
    'type': 'GLenum',
    'valid': [
      'GL_BYTE',
      'GL_UNSIGNED_BYTE',
      'GL_SHORT',
      'GL_UNSIGNED_SHORT',
    #  'GL_FIXED',  // This is not available on Desktop GL.
      'GL_FLOAT',
    ],
    'invalid': [
      'GL_DOUBLE',
    ],
  },
  'TextureBorder': {
    'type': 'GLint',
    'valid': [
      '0',
    ],
    'invalid': [
      '1',
    ],
  },
  'VertexAttribSize': {
    'type': 'GLint',
    'valid': [
      '1',
      '2',
      '3',
      '4',
    ],
    'invalid': [
      '0',
      '5',
    ],
  },
  'ZeroOnly': {
    'type': 'GLint',
    'valid': [
      '0',
    ],
    'invalid': [
      '1',
    ],
  },
  'FalseOnly': {
    'type': 'GLboolean',
    'valid': [
      'false',
    ],
    'invalid': [
      'true',
    ],
  },
}

def SplitWords(input_string):
  """Transforms a input_string into a list of lower-case components.

  Args:
    input_string: the input string.

  Returns:
    a list of lower-case words.
  """
  if input_string.find('_') > -1:
    # 'some_TEXT_' -> 'some text'
    return input_string.replace('_', ' ').strip().lower().split()
  else:
    if re.search('[A-Z]', input_string) and re.search('[a-z]', input_string):
      # mixed case.
      # look for capitalization to cut input_strings
      # 'SomeText' -> 'Some Text'
      input_string = re.sub('([A-Z])', r' \1', input_string).strip()
      # 'Vector3' -> 'Vector 3'
      input_string = re.sub('([^0-9])([0-9])', r'\1 \2', input_string)
    return input_string.lower().split()


def Lower(words):
  """Makes a lower-case identifier from words.

  Args:
    words: a list of lower-case words.

  Returns:
    the lower-case identifier.
  """
  return '_'.join(words)


def ToUnderscore(input_string):
  """converts CamelCase to camel_case."""
  words = SplitWords(input_string)
  return Lower(words)


class CWriter(object):
  """Writes to a file formatting it for Google's style guidelines."""

  def __init__(self, filename):
    self.filename = filename
    self.file_num = 0
    self.content = []

  def SetFileNum(self, num):
    """Used to help write number files and tests."""
    self.file_num = num

  def Write(self, string, split=True):
    """Writes a string to a file spliting if it's > 80 characters."""
    lines = string.splitlines()
    num_lines = len(lines)
    for ii in range(0, num_lines):
      self.__WriteLine(lines[ii], ii < (num_lines - 1) or string[-1] == '\n', split)

  def __FindSplit(self, string):
    """Finds a place to split a string."""
    splitter = string.find('=')
    if splitter >= 1 and not string[splitter + 1] == '=' and splitter < 80:
      return splitter
    # parts = string.split('(')
    parts = re.split("(?<=[^\"])\((?!\")", string)
    fptr = re.compile('\*\w*\)')
    if len(parts) > 1:
      splitter = len(parts[0])
      for ii in range(1, len(parts)):
        # Don't split on the dot in "if (.condition)".
        if (not parts[ii - 1][-3:] == "if " and
            # Don't split "(.)" or "(.*fptr)".
            (len(parts[ii]) > 0 and
                not parts[ii][0] == ")" and not fptr.match(parts[ii]))
            and splitter < 80):
          return splitter
        splitter += len(parts[ii]) + 1
    done = False
    end = len(string)
    last_splitter = -1
    while not done:
      splitter = string[0:end].rfind(',')
      if splitter < 0 or (splitter > 0 and string[splitter - 1] == '"'):
        return last_splitter
      elif splitter >= 80:
        end = splitter
      else:
        return splitter

  def __WriteLine(self, line, ends_with_eol, split):
    """Given a signle line, writes it to a file, splitting if it's > 80 chars"""
    if len(line) >= 80:
      if split:
          i = self.__FindSplit(line)
      else:
          i = 0
      if i > 0:
        line1 = line[0:i + 1]
        if line1[-1] == ' ':
          line1 = line1[:-1]
        lineend = ''
        if line1[0] == '#':
          lineend = ' \\'
        nolint = ''
        self.__AddLine(line1 + nolint + lineend + '\n')
        match = re.match("( +)", line1)
        indent = ""
        if match:
          indent = match.group(1)
        splitter = line[i]
        if not splitter == ',':
          indent = "    " + indent
        self.__WriteLine(indent + line[i + 1:].lstrip(), True, split)
        return
    nolint = ''
    self.__AddLine(line + nolint)
    if ends_with_eol:
      self.__AddLine('\n')

  def __AddLine(self, line):
    self.content.append(line)

  def Close(self):
    """Close the file."""
    content = "".join(self.content)
    write_file = True
    if os.path.exists(self.filename):
      old_file = open(self.filename, "rb");
      old_content = old_file.read()
      old_file.close();
      if content == old_content:
        write_file = False
    if write_file:
      file = open(self.filename, "wb")
      file.write(content)
      file.close()


class CHeaderWriter(CWriter):
  """Writes a C Header file."""

  _non_alnum_re = re.compile(r'[^a-zA-Z0-9]')

  def __init__(self, filename, file_comment = None, guard_depth = 3):
    CWriter.__init__(self, filename)

    base = os.path.dirname(os.path.abspath(filename))
    for i in range(guard_depth):
      base = os.path.dirname(base)

    hpath = os.path.abspath(filename)[len(base) + 1:]
    self.guard = self._non_alnum_re.sub('_', hpath).upper() + '_'

    self.Write(_DO_NOT_EDIT_WARNING)
    if not file_comment == None:
      self.Write(file_comment)
    self.Write("#ifndef %s\n" % self.guard)
    self.Write("#define %s\n\n" % self.guard)

  def Close(self):
    self.Write("#endif  // %s\n\n" % self.guard)
    CWriter.Close(self)

class TypeHandler(object):
  """This class emits code for a particular type of function."""

  _remove_expected_call_re = re.compile(r'  EXPECT_CALL.*?;\n', re.S)

  def __init__(self):
    pass

  def InitFunction(self, func):
    """Add or adjust anything type specific for this function."""
    if func.GetInfo('needs_size'):
      func.AddCmdArg(DataSizeArgument('data_size'))

  def AddImmediateFunction(self, generator, func):
    """Adds an immediate version of a function."""
    # Generate an immediate command if there is only 1 pointer arg.
    immediate = func.GetInfo('immediate')  # can be True, False or None
    if immediate == True or immediate == None:
      if func.num_pointer_args == 1 or immediate:
        generator.AddFunction(ImmediateFunction(func))

  def WriteStruct(self, func, file):
    """Writes a structure that matches the arguments to a function."""
    comment = func.GetInfo('cmd_comment')
    if not comment == None:
      file.Write(comment)
    file.Write("typedef struct _command_%s {\n" % func.name.lower())
    file.Write("  command_t header;\n")
    args = func.GetOriginalArgs()
    for arg in args:
      type = arg.type.replace("const", "")
      file.Write("  %s %s;\n" % (type, arg.name))
    if func.return_type != "void":
      file.Write("  %s result;\n" % (func.return_type))
    file.Write("} command_%s_t;\n" % (func.name.lower()))
    file.Write("\n")

  def WriteStructFunctionPointer(self, func, file):
    file.Write(func.return_type)
    file.Write(" (*%s) (" % func.name)
    file.Write(func.MakeTypedOriginalArgString(""), split=False)
    file.Write(");")

  def WriteCommandInitArgumentCopy(self, func, arg, file):
    # FIXME: Handle constness more gracefully.
    if func.IsSynchronous():
      type = arg.type.replace("const", "")
      file.Write("    command->%s = (%s) %s;\n" % (arg.name, type, arg.name))
      return

    if arg.type.find("char*") != -1:
      file.Write("    command->%s = strdup (%s);\n" % (arg.name, arg.name))
      return

    # We only need to copy arguments if the function is asynchronous.
    if  (arg.name in func.info.argument_has_size or \
         arg.name in func.info.argument_element_size or \
         arg.name in func.info.argument_size_from_function):

      components = []
      if arg.name in func.info.argument_has_size:
          components.append(func.info.argument_has_size[arg.name])
      if arg.name in func.info.argument_element_size:
          components.append("%i" % func.info.argument_element_size[arg.name])
      if arg.name in func.info.argument_size_from_function:
          components.append("%s (%s)" % (func.info.argument_size_from_function[arg.name], arg.name))
      if arg.type.find("void*") == -1:
          components.append("sizeof (%s)" % arg.type)
      arg_size = " * ".join(components)

      file.Write("    if (%s) {\n" % (arg.name))
      file.Write("        command->%s = malloc (%s);\n" % (arg.name, arg_size))
      file.Write("        memcpy (command->%s, %s, %s);\n" % (arg.name, arg.name, arg_size))
      file.Write("    } else\n")
      file.Write("        command->%s = 0;\n" % (arg.name))
      return

    # FIXME: Handle constness more gracefully.
    type = arg.type.replace("const", "")
    file.Write("    command->%s = (%s) %s;\n" % (arg.name, type, arg.name))

  def WriteCommandInit(self, func, file):
    file.Write ("inline ")
    self.WriteInitSignature(func, file)
    file.Write("\n{\n")

    args = func.GetOriginalArgs()
    if len(args):
        subclass_command_type = "command_%s_t" % func.name.lower()
        file.Write("    %s *command = (%s *) abstract_command;\n" % (subclass_command_type, subclass_command_type))

        for arg in args:
            self.WriteCommandInitArgumentCopy(func, arg, file)

        if func.return_type != "void":
          file.Write("    command->result = 0;\n")

    file.Write("}\n\n")

  def WriteCommandDestroy(self, func, file):
    file.Write("void\n");
    file.Write("command_%s_destroy_arguments (command_%s_t *command)\n" % \
        (func.name.lower(), func.name.lower()))
    file.Write("\n{\n")

    # The only thing we do for the moment is free arguments.
    arguments_to_free = [arg for arg in func.GetOriginalArgs() if arg.IsPointer()]
    for arg in arguments_to_free:
      file.Write("    if (command->%s)\n" % arg.name)
      file.Write("        free (command->%s);\n" % arg.name)
    file.Write("}\n")

  def WriteInitSignature(self, func, file):
    """Writes the declaration of the init function for a given function."""
    file.Write("void\n") 

    call = "command_%s_init (" % func.name.lower()
    file.Write(call)
    file.Write("command_t *abstract_command")

    file.Write(func.MakeTypedOriginalArgString("", separator=",\n" + (" " * len(call)),
                                               add_separator = True))
    file.Write(")")

  def WriteEnumName(self, func, file):
    """Writes an enum name that matches the name of a function."""
    file.Write("COMMAND_" + func.name.upper() + ",\n")

  def WriteCmdSizeTest(self, func, file):
    """Writes the size test for a command."""
    file.Write("  EXPECT_EQ(sizeof(cmd), cmd.header.size * 4u);\n")

  def WriteClientGLReturnLog(self, func, file):
    """Writes the return value logging code."""
    if func.return_type != "void":
      file.Write('  GPU_CLIENT_LOG("return:" << result)\n')

  def WriteImmediateCmdComputeSize(self, func, file):
    """Writes the size computation code for the immediate version of a cmd."""
    file.Write("  static uint32_t ComputeSize(uint32_t size_in_bytes) {\n")
    file.Write("    return static_cast<uint32_t>(\n")
    file.Write("        sizeof(ValueType) +  // NOLINT\n")
    file.Write("        RoundSizeToMultipleOfEntries(size_in_bytes));\n")
    file.Write("  }\n")
    file.Write("\n")

  def WriteImmediateCmdSetHeader(self, func, file):
    """Writes the SetHeader function for the immediate version of a cmd."""
    file.Write("  void SetHeader(uint32_t size_in_bytes) {\n")
    file.Write("    header.SetCmdByTotalSize<ValueType>(size_in_bytes);\n")
    file.Write("  }\n")
    file.Write("\n")

  def WriteImmediateCmdSet(self, func, file):
    """Writes the Set function for the immediate version of a command."""
    raise NotImplementedError(func.name)


class CustomHandler(TypeHandler):
  """Handler for commands that are auto-generated but require minor tweaks."""

  def __init__(self):
    TypeHandler.__init__(self)

class FunctionInfo(object):
  """Holds info about a function."""

  def __init__(self, info, type_handler):
    for key in info:
      setattr(self, key, info[key])
    self.type_handler = type_handler
    if not 'type' in info:
      self.type = ''
    if not 'default_return' in info:
      self.default_return = ''
    if not 'argument_has_size' in info:
      self.argument_has_size = {}
    if not 'out_arguments' in info:
      self.out_arguments = []
    if not 'argument_element_size' in info:
      self.argument_element_size = {}
    if not 'argument_size_from_function' in info:
      self.argument_size_from_function = {}
    if not 'mapped_names' in info:
      self.mapped_names = {}

class Argument(object):
  """A class that represents a function argument."""

  cmd_type_map_ = {
    'GLenum': 'uint32_t',
    'GLuint': 'uint32_t',
    'GLboolean' : 'uint32_t',
    'GLbitfield' : 'uint32_t',
    'GLint': 'int32_t',
    'GLchar': 'int8_t',
    'GLintptr': 'int32_t',
    'GLsizei': 'int32_t',
    'GLsizeiptr': 'int32_t',
    'GLfloat': 'float',
    'GLclampf': 'float',
    'GLvoid': 'void',
    'GLeglImageOES' : 'void*'
  }
  need_validation_ = ['GLsizei*', 'GLboolean*', 'GLenum*', 'GLint*']

  def __init__(self, name, type):
    self.name = name
    self.optional = type.endswith("Optional*")
    if self.optional:
      type = type[:-9] + "*"
    self.type = type

    if type.endswith("*"):
      type = type[:-1]
      needs_asterisk = True
    else:
      needs_asterisk = False

    needs_const = False
    if type.startswith("const"):
      type = type[6:]
      needs_const = True

    if type in self.cmd_type_map_:
      self.cmd_type = self.cmd_type_map_[type]
      needs_const = False
    else:
      self.cmd_type = type

    if needs_asterisk:
      self.cmd_type += "*"

    if needs_const:
      self.cmd_type = "const " + self.cmd_type

  def IsPointer(self):
    """Returns true if argument is a pointer."""
    return self.type.find("*") != -1

  def IsDoublePointer(self):
    """Returns true if argument is a double-pointer."""
    return self.type.find("**") != -1

  def IsString(self):
    """Returns true if argument is a double-pointer."""
    return self.type == "const char*" or self.type == "char*" or \
           self.type == "const GLchar*" or self.type == "GLchar*"

  def AddCmdArgs(self, args):
    """Adds command arguments for this argument to the given list."""
    return args.append(self)

  def AddInitArgs(self, args):
    """Adds init arguments for this argument to the given list."""
    return args.append(self)

  def GetValidArg(self, func, offset, index):
    """Gets a valid value for this argument."""
    valid_arg = func.GetValidArg(offset)
    if valid_arg != None:
      return valid_arg
    return str(offset + 1)

  def GetValidClientSideArg(self, func, offset, index):
    """Gets a valid value for this argument."""
    return str(offset + 1)

  def GetValidClientSideCmdArg(self, func, offset, index):
    """Gets a valid value for this argument."""
    return str(offset + 1)

  def GetValidGLArg(self, func, offset, index):
    """Gets a valid GL value for this argument."""
    valid_arg = func.GetValidArg(offset)
    if valid_arg != None:
      return valid_arg
    return str(offset + 1)

  def GetNumInvalidValues(self, func):
    """returns the number of invalid values to be tested."""
    return 0

  def GetInvalidArg(self, offset, index):
    """returns an invalid value and expected parse result by index."""
    return ("---ERROR0---", "---ERROR2---", None)

  def WriteGetAddress(self, file):
    """Writes the code to get the address this argument refers to."""
    pass

  def GetImmediateVersion(self):
    """Gets the immediate version of this argument."""
    return self


class BoolArgument(Argument):
  """class for GLboolean"""

  def __init__(self, name, type):
    Argument.__init__(self, name, 'GLboolean')

  def GetValidArg(self, func, offset, index):
    """Gets a valid value for this argument."""
    return 'true'

  def GetValidClientSideArg(self, func, offset, index):
    """Gets a valid value for this argument."""
    return 'true'

  def GetValidClientSideCmdArg(self, func, offset, index):
    """Gets a valid value for this argument."""
    return 'true'

  def GetValidGLArg(self, func, offset, index):
    """Gets a valid GL value for this argument."""
    return 'true'


class UniformLocationArgument(Argument):
  """class for uniform locations."""

  def __init__(self, name):
    Argument.__init__(self, name, "GLint")

  def GetValidArg(self, func, offset, index):
    """Gets a valid value for this argument."""
    return "%d" % (offset + 1)


class DataSizeArgument(Argument):
  """class for data_size which Bucket commands do not need."""

  def __init__(self, name):
    Argument.__init__(self, name, "uint32_t")


class SizeArgument(Argument):
  """class for GLsizei and GLsizeiptr."""

  def __init__(self, name, type):
    Argument.__init__(self, name, type)

  def GetNumInvalidValues(self, func):
    """overridden from Argument."""
    if func.is_immediate:
      return 0
    return 1

  def GetInvalidArg(self, offset, index):
    """overridden from Argument."""
    return ("-1", "kNoError", "GL_INVALID_VALUE")

class SizeNotNegativeArgument(SizeArgument):
  """class for GLsizeiNotNegative. It's NEVER allowed to be negative"""

  def __init__(self, name, type, gl_type):
    SizeArgument.__init__(self, name, gl_type)

  def GetInvalidArg(self, offset, index):
    """overridden from SizeArgument."""
    return ("-1", "kOutOfBounds", "GL_NO_ERROR")


class EnumBaseArgument(Argument):
  """Base class for EnumArgument, IntArgument and ValidatedBoolArgument"""

  def __init__(self, name, gl_type, type, gl_error):
    Argument.__init__(self, name, gl_type)

    self.local_type = type
    self.gl_error = gl_error
    name = type[len(gl_type):]
    self.type_name = name
    self.enum_info = _ENUM_LISTS[name]

  def GetValidArg(self, func, offset, index):
    valid_arg = func.GetValidArg(offset)
    if valid_arg != None:
      return valid_arg
    if 'valid' in self.enum_info:
      valid = self.enum_info['valid']
      num_valid = len(valid)
      if index >= num_valid:
        index = num_valid - 1
      return valid[index]
    return str(offset + 1)

  def GetValidClientSideArg(self, func, offset, index):
    """Gets a valid value for this argument."""
    return self.GetValidArg(func, offset, index)

  def GetValidClientSideCmdArg(self, func, offset, index):
    """Gets a valid value for this argument."""
    return self.GetValidArg(func, offset, index)

  def GetValidGLArg(self, func, offset, index):
    """Gets a valid value for this argument."""
    return self.GetValidArg(func, offset, index)

  def GetNumInvalidValues(self, func):
    """returns the number of invalid values to be tested."""
    if 'invalid' in self.enum_info:
      invalid = self.enum_info['invalid']
      return len(invalid)
    return 0

  def GetInvalidArg(self, offset, index):
    """returns an invalid value by index."""
    if 'invalid' in self.enum_info:
      invalid = self.enum_info['invalid']
      num_invalid = len(invalid)
      if index >= num_invalid:
        index = num_invalid - 1
      return (invalid[index], "kNoError", self.gl_error)
    return ("---ERROR1---", "kNoError", self.gl_error)


class EnumArgument(EnumBaseArgument):
  """A class that represents a GLenum argument"""

  def __init__(self, name, type):
    EnumBaseArgument.__init__(self, name, "GLenum", type, "GL_INVALID_ENUM")

class IntArgument(EnumBaseArgument):
  """A class for a GLint argument that can only except specific values.

  For example glTexImage2D takes a GLint for its internalformat
  argument instead of a GLenum.
  """

  def __init__(self, name, type):
    EnumBaseArgument.__init__(self, name, "GLint", type, "GL_INVALID_VALUE")


class ValidatedBoolArgument(EnumBaseArgument):
  """A class for a GLboolean argument that can only except specific values.

  For example glUniformMatrix takes a GLboolean for it's transpose but it
  must be false.
  """

  def __init__(self, name, type):
    EnumBaseArgument.__init__(self, name, "GLboolean", type, "GL_INVALID_VALUE")

class ImmediatePointerArgument(Argument):
  """A class that represents an immediate argument to a function.

  An immediate argument is one where the data follows the command.
  """

  def __init__(self, name, type):
    Argument.__init__(self, name, type)

  def AddCmdArgs(self, args):
    """Overridden from Argument."""
    pass

  def GetImmediateVersion(self):
    """Overridden from Argument."""
    return None


class PointerArgument(Argument):
  """A class that represents a pointer argument to a function."""

  def __init__(self, name, type):
    Argument.__init__(self, name, type)

  def IsPointer(self):
    """Returns true if argument is a pointer."""
    return True

  def GetValidArg(self, func, offset, index):
    """Overridden from Argument."""
    return "shared_memory_id_, shared_memory_offset_"

  def GetValidGLArg(self, func, offset, index):
    """Overridden from Argument."""
    return "reinterpret_cast<%s>(shared_memory_address_)" % self.type

  def GetNumInvalidValues(self, func):
    """Overridden from Argument."""
    return 2

  def GetInvalidArg(self, offset, index):
    """Overridden from Argument."""
    if index == 0:
      return ("kInvalidSharedMemoryId, 0", "kOutOfBounds", None)
    else:
      return ("shared_memory_id_, kInvalidSharedMemoryOffset",
              "kOutOfBounds", None)

  def AddCmdArgs(self, args):
    """Overridden from Argument."""
    args.append(Argument(self.name, self.type))

  def WriteGetCode(self, file):
    """Overridden from Argument."""
    file.Write(
        "  %s %s = GetSharedMemoryAs<%s>(\n" %
        (self.type, self.name, self.type))
    file.Write(
        "      c.%s_shm_id, c.%s_shm_offset, data_size);\n" %
        (self.name, self.name))

  def WriteGetAddress(self, file):
    """Overridden from Argument."""
    file.Write(
        "  %s %s = GetSharedMemoryAs<%s>(\n" %
        (self.type, self.name, self.type))
    file.Write(
        "      %s_shm_id, %s_shm_offset, %s_size);\n" %
        (self.name, self.name, self.name))

  def GetImmediateVersion(self):
    """Overridden from Argument."""
    return ImmediatePointerArgument(self.name, self.type)


class NonImmediatePointerArgument(PointerArgument):
  """A pointer argument that stays a pointer even in an immediate cmd."""

  def __init__(self, name, type):
    PointerArgument.__init__(self, name, type)

  def IsPointer(self):
    """Returns true if argument is a pointer."""
    return False

  def GetImmediateVersion(self):
    """Overridden from Argument."""
    return self


class ResourceIdArgument(Argument):
  """A class that represents a resource id argument to a function."""

  def __init__(self, name, type):
    match = re.match("(GLid\w+)", type)
    self.resource_type = match.group(1)[4:]
    type = type.replace(match.group(1), "GLuint")
    Argument.__init__(self, name, type)

  def WriteGetCode(self, file):
    """Overridden from Argument."""
    file.Write("  %s %s = c.%s;\n" % (self.type, self.name, self.name))

  def GetValidArg(self, func, offset, index):
    return "client_%s_id_" % self.resource_type.lower()

  def GetValidGLArg(self, func, offset, index):
    return "kService%sId" % self.resource_type


class ResourceIdBindArgument(Argument):
  """Represents a resource id argument to a bind function."""

  def __init__(self, name, type):
    match = re.match("(GLidBind\w+)", type)
    self.resource_type = match.group(1)[8:]
    type = type.replace(match.group(1), "GLuint")
    Argument.__init__(self, name, type)

  def WriteGetCode(self, file):
    """Overridden from Argument."""
    code = """  %(type)s %(name)s = c.%(name)s;
"""
    file.Write(code % {'type': self.type, 'name': self.name})

  def GetValidArg(self, func, offset, index):
    return "client_%s_id_" % self.resource_type.lower()

  def GetValidGLArg(self, func, offset, index):
    return "kService%sId" % self.resource_type


class ResourceIdZeroArgument(Argument):
  """Represents a resource id argument to a function that can be zero."""

  def __init__(self, name, type):
    match = re.match("(GLidZero\w+)", type)
    self.resource_type = match.group(1)[8:]
    type = type.replace(match.group(1), "GLuint")
    Argument.__init__(self, name, type)

  def WriteGetCode(self, file):
    """Overridden from Argument."""
    file.Write("  %s %s = c.%s;\n" % (self.type, self.name, self.name))

  def GetValidArg(self, func, offset, index):
    return "client_%s_id_" % self.resource_type.lower()

  def GetValidGLArg(self, func, offset, index):
    return "kService%sId" % self.resource_type

  def GetNumInvalidValues(self, func):
    """returns the number of invalid values to be tested."""
    return 1

  def GetInvalidArg(self, offset, index):
    """returns an invalid value by index."""
    return ("kInvalidClientId", "kNoError", "GL_INVALID_VALUE")


class Function(object):
  """A class that represents a function."""

  def __init__(self, original_name, name, info, return_type, original_args,
               args_for_cmds, cmd_args, init_args, num_pointer_args):
    self.name = name
    self.original_name = original_name
    self.info = info
    self.type_handler = info.type_handler
    self.return_type = return_type
    self.original_args = original_args
    self.num_pointer_args = num_pointer_args
    self.can_auto_generate = num_pointer_args == 0 and return_type == "void"
    self.cmd_args = cmd_args
    self.init_args = init_args
    self.InitFunction()
    self.args_for_cmds = args_for_cmds
    self.is_immediate = False

  def IsType(self, type_name):
    """Returns true if function is a certain type."""
    return self.info.type == type_name

  def InitFunction(self):
    """Calls the init function for the type handler."""
    self.type_handler.InitFunction(self)

  def GetInfo(self, name):
    """Returns a value from the function info for this function."""
    if hasattr(self.info, name):
      return getattr(self.info, name)
    return None

  def GetValidArg(self, index):
    """Gets a valid arg from the function info if one exists."""
    valid_args = self.GetInfo('valid_args')
    if valid_args and str(index) in valid_args:
      return valid_args[str(index)]
    return None

  def AddInfo(self, name, value):
    """Adds an info."""
    setattr(self.info, name, value)

  def IsCoreGLFunction(self):
    return (not self.GetInfo('extension') and
            not self.GetInfo('pepper_interface'))

  def IsSynchronous(self):
    if self.IsType('Asynchronous'):
        return False
    else:
        return self.IsType('Synchronous') \
            or self.HasReturnValue() \
            or len(self.info.out_arguments) > 0

  def HasReturnValue(self):
    return self.return_type != 'void'

  def IsOutArgument(self, arg):
    """Returns true if given argument is an out argument for this function"""
    return arg.name in self.info.out_arguments

  def GetGLFunctionName(self):
    """Gets the function to call to execute GL for this command."""
    if self.GetInfo('decoder_func'):
      return self.GetInfo('decoder_func')
    return "gl%s" % self.original_name

  def GetGLTestFunctionName(self):
    gl_func_name = self.GetInfo('gl_test_func')
    if gl_func_name == None:
      gl_func_name = self.GetGLFunctionName()
    if gl_func_name.startswith("gl"):
      gl_func_name = gl_func_name[2:]
    else:
      gl_func_name = self.original_name
    return gl_func_name

  def AddCmdArg(self, arg):
    """Adds a cmd argument to this function."""
    self.cmd_args.append(arg)

  def GetCmdArgs(self):
    """Gets the command args for this function."""
    return self.cmd_args

  def ClearCmdArgs(self):
    """Clears the command args for this function."""
    self.cmd_args = []

  def GetInitArgs(self):
    """Gets the init args for this function."""
    return self.init_args

  def GetOriginalArgs(self):
    """Gets the original arguments to this function."""
    return self.original_args

  def GetLastOriginalArg(self):
    """Gets the last original argument to this function."""
    return self.original_args[len(self.original_args) - 1]

  def __GetArgList(self, arg_string, add_separator, separator):
    """Adds a comma if arg_string is not empty and add_comma is true."""
    prefix = ""
    if add_separator and len(arg_string):
      prefix = separator
    return "%s%s" % (prefix, arg_string)

  def MakeDefaultReturnStatement(self):
    """Make a return statement with the default value."""
    if self.return_type == "void":
        return "return"
    elif self.info.default_return:
        return "return %s" % self.info.default_return
    else:
        return "return %s" % _DEFAULT_RETURN_VALUES[self.return_type]

  def MakeTypedOriginalArgString(self, prefix, add_separator = False, separator = ", "):
    """Gets a list of arguments as they arg in GL."""
    args = self.GetOriginalArgs()
    arg_string = separator.join(
        ["%s%s %s" % (prefix, arg.type, arg.name) for arg in args])
    return self.__GetArgList(arg_string, add_separator, separator)

  def MakeOriginalArgString(self, prefix, add_separator = False, separator = ", "):
    """Gets the list of arguments as they are in GL."""
    args = self.GetOriginalArgs()
    arg_string = separator.join(
        ["%s%s" % (prefix, arg.name) for arg in args])
    return self.__GetArgList(arg_string, add_separator, separator)

  def WriteStruct(self, file):
    self.type_handler.WriteStruct(self, file)

  def WriteStructFunctionPointer(self, file):
    self.type_handler.WriteStructFunctionPointer(self, file)

  def WriteCommandInit(self, file):
    self.type_handler.WriteCommandInit(self, file)

  def WriteCommandDestroy(self, file):
    self.type_handler.WriteCommandDestroy(self, file)

  def WriteInitSignature(self, file):
    self.type_handler.WriteInitSignature(self, file)

  def WriteEnumName(self, file):
    self.type_handler.WriteEnumName(self, file)

  def KnowHowToPassArguments(self):
    # Passing argument is quite easy for synchronous functions, because
    # we can just pass the pointers without doing a copy.
    if self.IsSynchronous():
        return True
    if self.IsType('Passthrough'):
        return True

    for arg in self.GetOriginalArgs():
        if self.IsOutArgument(arg):
            continue
        if arg.name in self.info.out_arguments:
            continue
        if arg.name in self.info.argument_has_size:
            continue
        if arg.name in self.info.argument_element_size:
            continue
        if arg.name in self.info.argument_size_from_function:
            continue
        if arg.IsString():
            continue
        if arg.IsPointer():
            return False
    return True

  def KnowHowToAssignDefaultReturnValue(self):
    return not self.HasReturnValue() or \
           self.return_type in _DEFAULT_RETURN_VALUES or \
           self.info.default_return

  def NeedsDestructor(self):
    return not self.IsSynchronous() and not self.IsType('Passthrough')

  def IsExtensionFunction(self):
    return self.name.endswith("OES") or \
           self.name.endswith("QCOM") or \
           self.name.endswith("APPLE") or \
           self.name.endswith("ANGLE") or \
           self.name.endswith("NV") or \
           self.name.endswith("IMG") or \
           self.name.endswith("EXT") or \
           self.name.endswith("ARB") or \
           self.name.endswith("AMD") or \
           self.name.endswith("MESA") or \
           self.name.endswith("SEC") or \
           self.name.endswith("KHR") or \
           self.name.endswith("HI")

  def ShouldHideEntryPoint(self):
    return self.IsExtensionFunction() and not self.name.startswith("egl")

  def GetMappedNameType(self):
    if ('type' in self.info.mapped_names):
        return self.info.mapped_names['type']
    return ''

  def GetMappedNameAttributes(self):
    if ('attrib_list' in self.info.mapped_names):
        return self.info.mapped_names['attrib_list']
    return []

  def NeedsCreateMappedName(self, mapped_name):
    if ('create_attrib_if_needed' in self.info.mapped_names):
        return mapped_name in self.info.mapped_names['create_attrib_if_needed']
    return False

class ImmediateFunction(Function):
  """A class that represnets an immediate function command."""

  def __init__(self, func):
    new_args = []
    for arg in func.GetOriginalArgs():
      new_arg = arg.GetImmediateVersion()
      if new_arg:
        new_args.append(new_arg)

    cmd_args = []
    new_args_for_cmds = []
    for arg in func.args_for_cmds:
      new_arg = arg.GetImmediateVersion()
      if new_arg:
        new_args_for_cmds.append(new_arg)
        new_arg.AddCmdArgs(cmd_args)

    new_init_args = []
    for arg in new_args_for_cmds:
      arg.AddInitArgs(new_init_args)

    Function.__init__(
        self,
        func.original_name,
        "%sImmediate" % func.name,
        func.info,
        func.return_type,
        new_args,
        new_args_for_cmds,
        cmd_args,
        new_init_args,
        0)
    self.is_immediate = True

def CreateArg(arg_string):
  """Creates an Argument."""
  arg_parts = arg_string.split()
  if len(arg_parts) == 1 and arg_parts[0] == 'void':
    return None
  # Is this a pointer argument?
  elif arg_string.find('*') >= 0:
    if arg_parts[0] == 'NonImmediate':
      return NonImmediatePointerArgument(
          arg_parts[-1],
          " ".join(arg_parts[1:-1]))
    else:
      return PointerArgument(
          arg_parts[-1],
          " ".join(arg_parts[0:-1]))
  # Is this a resource argument? Must come after pointer check.
  elif arg_parts[0].startswith('GLidBind'):
    return ResourceIdBindArgument(arg_parts[-1], " ".join(arg_parts[0:-1]))
  elif arg_parts[0].startswith('GLidZero'):
    return ResourceIdZeroArgument(arg_parts[-1], " ".join(arg_parts[0:-1]))
  elif arg_parts[0].startswith('GLid'):
    return ResourceIdArgument(arg_parts[-1], " ".join(arg_parts[0:-1]))
  elif arg_parts[0].startswith('GLenum') and len(arg_parts[0]) > 6:
    return EnumArgument(arg_parts[-1], " ".join(arg_parts[0:-1]))
  elif arg_parts[0].startswith('GLboolean') and len(arg_parts[0]) > 9:
    return ValidatedBoolArgument(arg_parts[-1], " ".join(arg_parts[0:-1]))
  elif arg_parts[0].startswith('GLboolean'):
    return BoolArgument(arg_parts[-1], " ".join(arg_parts[0:-1]))
  elif arg_parts[0].startswith('GLintUniformLocation'):
    return UniformLocationArgument(arg_parts[-1])
  elif (arg_parts[0].startswith('GLint') and len(arg_parts[0]) > 5 and
        not arg_parts[0].startswith('GLintptr')):
    return IntArgument(arg_parts[-1], " ".join(arg_parts[0:-1]))
  elif (arg_parts[0].startswith('GLsizeiNotNegative') or
        arg_parts[0].startswith('GLintptrNotNegative')):
    return SizeNotNegativeArgument(arg_parts[-1],
                                   " ".join(arg_parts[0:-1]),
                                   arg_parts[0][0:-11])
  elif arg_parts[0].startswith('GLsize'):
    return SizeArgument(arg_parts[-1], " ".join(arg_parts[0:-1]))
  else:
    return Argument(arg_parts[-1], " ".join(arg_parts[0:-1]))


class GLGenerator(object):
  """A class to generate GL command buffers."""

  _gles2_function_re = re.compile(r'GL_APICALL(.*?)GL_APIENTRY (.*?) \((.*?)\);')
  _egl_function_re = re.compile(r'EGLAPI(.*?)EGLAPIENTRY (.*?) \((.*?)\);')

  def __init__(self, verbose):
    self.original_functions = []
    self.functions = []
    self.verbose = verbose
    self.errors = 0
    self._function_info = {}
    self._empty_type_handler = TypeHandler()
    self._empty_function_info = FunctionInfo({}, self._empty_type_handler)
    self.pepper_interfaces = []
    self.interface_info = {}
    self.command_custom_text = None
    self.command_custom_header_text = None
    self.server_text = None

    for func_name in _FUNCTION_INFO:
      info = _FUNCTION_INFO[func_name]
      type = ''
      if 'type' in info:
        type = info['type']
      self._function_info[func_name] = FunctionInfo(info, self._empty_type_handler)

  def AddFunction(self, func):
    """Adds a function."""
    self.functions.append(func)

  def GetTypeHandler(self, name):
    """Gets a type info for the given type."""
    if name in self._type_handlers:
      return self._type_handlers[name]
    return self._empty_type_handler

  def GetFunctionInfo(self, name):
    """Gets a type info for the given function name."""
    if name in self._function_info:
      return self._function_info[name]
    return self._empty_function_info

  def Log(self, msg):
    """Prints something if verbose is true."""
    if self.verbose:
      print msg

  def Error(self, msg):
    """Prints an error."""
    print "Error: %s" % msg
    self.errors += 1

  def WriteNamespaceOpen(self, file):
    """Writes the code for the namespace."""
    file.Write("namespace gpu {\n")
    file.Write("namespace gles2 {\n")
    file.Write("\n")

  def WriteNamespaceClose(self, file):
    """Writes the code to close the namespace."""
    file.Write("}  // namespace gles2\n")
    file.Write("}  // namespace gpu\n")
    file.Write("\n")

  def ParseArgs(self, arg_string):
    """Parses a function arg string."""
    args = []
    num_pointer_args = 0
    parts = arg_string.split(',')
    is_gl_enum = False
    for arg_string in parts:
      if arg_string.startswith('GLenum '):
        is_gl_enum = True
      arg = CreateArg(arg_string)
      if arg:
        args.append(arg)
        if arg.IsPointer():
          num_pointer_args += 1
    return (args, num_pointer_args, is_gl_enum)

  def ParseAPIFile(self, filename, regular_expression):
    """Parses the cmd_buffer_functions.txt file and extracts the functions"""

    f = open(filename, "r")
    functions = f.read()
    f.close()

    for line in functions.splitlines():
      match = regular_expression.match(line)
      if match:
        func_name = match.group(2)
        func_info = self.GetFunctionInfo(func_name)
        if func_info.type != 'Noop':
          return_type = match.group(1).strip()
          arg_string = match.group(3)
          (args, num_pointer_args, is_gl_enum) = self.ParseArgs(arg_string)
          # comment in to find out which functions use bare enums.
          # if is_gl_enum:
          #   self.Log("%s uses bare GLenum" % func_name)
          args_for_cmds = args
          if hasattr(func_info, 'cmd_args'):
            (args_for_cmds, num_pointer_args, is_gl_enum) = (
                self.ParseArgs(getattr(func_info, 'cmd_args')))
          cmd_args = []
          for arg in args_for_cmds:
            arg.AddCmdArgs(cmd_args)
          init_args = []
          for arg in args_for_cmds:
            arg.AddInitArgs(init_args)
          return_arg = CreateArg(return_type + " result")
          if return_arg:
            init_args.append(return_arg)
          f = Function(func_name, func_name, func_info, return_type, args,
                       args_for_cmds, cmd_args, init_args, num_pointer_args)
          self.original_functions.append(f)
          gen_cmd = f.GetInfo('gen_cmd')
          if gen_cmd == True or gen_cmd == None:
            self.AddFunction(f)

    self.Log("Auto Generated Functions    : %d" %
             len([f for f in self.functions if f.can_auto_generate or
                  (not f.IsType('') and not f.IsType('Custom') and
                   not f.IsType('Todo'))]))

    funcs = [f for f in self.functions if not f.can_auto_generate and
             (f.IsType('') or f.IsType('Custom') or f.IsType('Todo'))]
    self.Log("Non Auto Generated Functions: %d" % len(funcs))

    for f in funcs:
      self.Log("  %-10s %-20s gl%s" % (f.info.type, f.return_type, f.name))

  def ParseAPIFiles(self):
    self.ParseAPIFile("gles2_functions.txt", self._gles2_function_re)
    self.ParseAPIFile("egl_functions.txt", self._egl_function_re)

  def HasCustomClientEntryPoint(self, func):
    if func.IsType('Manual'):
        return True

    # Manual init functions always allow generating the client-side entry point.
    if self.HasCustomInit(func):
        return False
    if func.IsSynchronous():
        return False
    if not func.KnowHowToPassArguments():
        return True
    return not func.KnowHowToAssignDefaultReturnValue()

  def GetClientEntryPointSignature(self, func):
    if self.HasCustomClientEntryPoint(func):
        return

    args = func.MakeTypedOriginalArgString("")
    if not args:
        args = "void"

    name = func.name
    if func.ShouldHideEntryPoint():
        name = "__hidden_gpuproxy_" + name

    return "%s %s (%s)" % (func.return_type, name, args)

  def WriteClientEntryPoints(self, filename):
    file = CWriter(filename)
    file.Write('#include "caching_client.h"\n')
    self.WriteGLHeaders(file)

    for func in self.functions:
        if func.name.find("eglGetProcAddress") != -1:
            continue

        header = self.GetClientEntryPointSignature(func)
        if not header:
            continue

        file.Write(header + "\n")
        file.Write("{\n")
        file.Write("    INSTRUMENT();\n")

        file.Write("    if (should_use_base_dispatch ()) {\n")

        file.Write("        ")
        if func.HasReturnValue():
            file.Write("return ")
        file.Write("dispatch_table_get_base ()->%s (NULL" % func.name)
        file.Write(func.MakeOriginalArgString("", add_separator=True))
        file.Write(");\n")
        if not func.HasReturnValue():
            file.Write("        return;\n")

        file.Write("    }\n")

        file.Write("    client_t *client = client_get_thread_local ();\n");

        file.Write("    ")
        if func.HasReturnValue():
            file.Write("return ")
        file.Write("client->dispatch.%s (client" % func.name)
        file.Write(func.MakeOriginalArgString("", separator=", ", add_separator=True))
        file.Write(");\n")

        file.Write("}\n\n")

    file.Close()

  def WriteBaseClient(self, filename):
    """Writes the base server implementation, which just places the commands on the command buffer and runs them."""
    file = CWriter(filename)
    self.WriteGLHeaders(file)

    for func in self.functions:
        file.Write("static %s\n" % func.return_type)
        file.Write("client_dispatch_%s (void* object" % func.name.lower())
        file.Write(func.MakeTypedOriginalArgString("", separator=",\n    ", add_separator=True))
        file.Write(")\n")

        file.Write("{\n")

        if func.name in FUNCTIONS_GENERATING_ERRORS:
            file.Write("    egl_state_t *state = client_get_current_state (CLIENT (object));\n");
            file.Write("    if (state)\n");
            file.Write("        state->need_get_error = true;\n\n");

        file.Write("    INSTRUMENT();\n");
        file.Write("    command_t *command = client_get_space_for_command (COMMAND_%s);\n" % func.name.upper())

        header = "    command_%s_init (" % func.name.lower()
        indent = " " * len(header)
        file.Write(header + "command")
        args = func.MakeOriginalArgString(indent, separator = ",\n", add_separator = True)
        if args:
            file.Write(args)
        file.Write(");\n\n")

        if func.IsSynchronous():
            file.Write("    client_run_command (command);\n");
        else:
            file.Write("    client_run_command_async (command);\n");

        if func.HasReturnValue():
          file.Write("\n")
          file.Write("    return ((command_%s_t *)command)->result;\n\n" % func.name.lower())

        file.Write("}\n\n")

    file.Write("void\n")
    file.Write("client_fill_dispatch_table (dispatch_table_t *dispatch)\n")
    file.Write("{\n")
    for func in self.functions:
        file.Write('    dispatch->%s = client_dispatch_%s;\n' % (func.name, func.name.lower()))
    file.Write("}\n")
    file.Close()

  def WriteCommandHeader(self, filename):
    """Writes the command format"""
    file = CWriter(filename)

    file.Write("#ifndef COMMAND_AUTOGEN_H\n")
    file.Write("#define COMMAND_AUTOGEN_H\n\n")
    file.Write("#include \"command.h\"\n\n")
    file.Write("#include <EGL/egl.h>\n")
    file.Write("#include <EGL/eglext.h>\n")
    file.Write("#include <GLES2/gl2.h>\n")
    file.Write("#include <GLES2/gl2ext.h>\n\n")

    for func in self.functions:
      if self.HasCustomStruct(func):
        continue
      func.WriteStruct(file)

    file.Write("\n")

    for func in self.functions:
      file.Write("private ");
      func.WriteInitSignature(file)
      file.Write(";\n\n")

      if func.NeedsDestructor() or self.HasCustomDestroyArguments(func):
        file.Write("private void\n");
        file.Write("command_%s_destroy_arguments (command_%s_t *command);\n\n" % \
          (func.name.lower(), func.name.lower()))

    file.Write("#endif /*COMMAND_AUTOGEN_H*/\n")
    file.Close()

  def WriteGLHeaders(self, file):
    # TODO: Eventually this should write either the EGL/GLES headers or those for GLX.
    file.Write("#include <EGL/egl.h>\n")
    file.Write("#include <EGL/eglext.h>\n")
    file.Write("#include <GLES2/gl2.h>\n")
    file.Write("#include <GLES2/gl2ext.h>\n\n")

  def CommandCustomText(self):
    if not self.command_custom_text:
        self.command_custom_text = open(os.path.join('..', 'command_custom.c')).read()
    return self.command_custom_text

  def CommandCustomHeaderText(self):
    if not self.command_custom_header_text:
        self.command_custom_header_text = open(os.path.join('..', 'command_custom.h')).read()
    return self.command_custom_header_text

  def ServerText(self):
    if not self.server_text:
        self.server_text = open(os.path.join('..', 'server', 'server.c')).read()
    return self.server_text

  def HasCustomInit(self, func):
    init_name = "command_%s_init " % func.name.lower()
    return self.CommandCustomText().find(init_name) != -1

  def HasCustomDestroyArguments(self, func):
    init_name = "command_%s_destroy_arguments " % func.name.lower()
    return self.CommandCustomText().find(init_name) != -1

  def HasCustomStruct(self, func):
    struct_declaration = "typedef struct _command_%s " % func.name.lower()
    return self.CommandCustomHeaderText().find(struct_declaration) != -1

  def HasCustomServerHandler(self, func):
    handler_name = "server_handle_%s " % func.name.lower()
    return self.ServerText().find(handler_name) != -1

  def WriteCommandInitilizationAndSizeFunction(self, filename):
    """Writes the command implementation for the client-side"""
    file = CWriter(filename)

    file.Write("#include \"command.h\"\n")
    file.Write("#include <string.h>\n\n")
    file.Write('#include "gles2_utils.h"\n')

    for func in self.functions:
      if not self.HasCustomInit(func):
        func.WriteCommandInit(file)
      if not self.HasCustomDestroyArguments(func):
        func.WriteCommandDestroy(file)

    file.Write("void\n")
    file.Write("command_initialize_sizes (size_t *sizes)\n")
    file.Write("{\n")
    for func in self.functions:
        file.Write("sizes[COMMAND_%s] = sizeof (command_%s_t);\n" % \
                    (func.name.upper(), func.name.lower()))
    file.Write("}\n")

    file.Write("\n")
    file.Close()

  def WriteDispatchTable(self, filename):
    """Writes the dispatch struct for the server-side"""
    file = CHeaderWriter(filename)

    file.Write("#include \"config.h\"\n")
    file.Write("#include \"compiler_private.h\"\n")
    self.WriteGLHeaders(file)

    file.Write("typedef void (*FunctionPointerType)(void);\n")
    file.Write("typedef struct _dispatch_table {\n")
    for func in self.functions:
        file.Write("    %s (*%s) (void *object" % (func.return_type, func.name))
        file.Write(func.MakeTypedOriginalArgString("", add_separator = True), split=False)
        file.Write(");\n")
    file.Write("} dispatch_table_t;")
    file.Write("\n")
    file.Close()

  def WriteBaseServer(self, filename):
    file = CWriter(filename)

    for func in self.functions:
        if self.HasCustomServerHandler(func):
            continue

        file.Write("static void\n")
        file.Write("server_handle_%s (server_t *server, command_t *abstract_command)\n" % func.name.lower())
        file.Write("{\n")
        file.Write("    INSTRUMENT ();\n")

        need_destructor_call = func.NeedsDestructor() or self.HasCustomDestroyArguments(func)
        if need_destructor_call or len(func.GetOriginalArgs()) > 0 or func.HasReturnValue():
          file.Write("    command_%s_t *command =\n" % func.name.lower())
          file.Write("            (command_%s_t *)abstract_command;\n" % func.name.lower())

        mapped_names = func.GetMappedNameAttributes()
        for mapped_name in mapped_names:
          file.Write("    if (command->%s) {\n" % mapped_name)
          file.Write("        mutex_lock (name_mapping_mutex);\n")
          file.Write("        GLuint *%s = hash_lookup (name_mapping_%s, command->%s);\n" % (mapped_name, func.GetMappedNameType(),  mapped_name))
          file.Write("        mutex_unlock (name_mapping_mutex);\n")
          file.Write("        if (!%s) {\n" % mapped_name)
          if (func.NeedsCreateMappedName(mapped_name)):
            file.Write("        GLuint *data = (GLuint *) malloc (1 * sizeof (GLuint));\n")
            file.Write("        *data = command->%s;\n" %mapped_name)
            file.Write("        hash_insert (name_mapping_%s, *data, data);\n" %func.GetMappedNameType())
            file.Write("        %s = data;\n" %mapped_name)
          else:
            file.Write("            return;\n")
          file.Write("        }\n");
          file.Write("        command->%s = *%s;\n" % (mapped_name, mapped_name))
          file.Write("    }\n")

        file.Write("    ")
        if func.HasReturnValue():
          file.Write("command->result = ")
        file.Write("server->dispatch.%s (server" % func.name)

        for arg in func.GetOriginalArgs():
          file.Write(", ")
          if arg.IsDoublePointer() and arg.type.find("const") != -1:
            file.Write("(%s) " % arg.type)
          file.Write("command->%s" % arg.name)
        file.Write(");\n")

        if need_destructor_call:
          file.Write("    command_%s_destroy_arguments (command);\n" % func.name.lower())
        file.Write("}\n\n")

    file.Write("static void\n")
    file.Write("server_fill_command_handler_table (server_t* server)\n")
    file.Write("{\n")
    for func in self.functions:
      file.Write("    server->handler_table[COMMAND_%s] = \n" % func.name.upper())
      file.Write("        server_handle_%s;\n" % func.name.lower())
    file.Write("}\n\n")

    file.Close()

  def WritePassthroughDispatchTableImplementation(self, filename):
    """Writes the pass-through dispatch table implementation."""
    file = CWriter(filename)

    self.WriteGLHeaders(file)
    file.Write("\n")

    for func in self.functions:
        file.Write("static %s (*real_%s) (" % (func.return_type, func.name))
        file.Write(func.MakeTypedOriginalArgString(""), split=False)
        file.Write(");\n")

    for func in self.functions:
        file.Write("static %s\n" % func.return_type)

        func_name = "passthrough_%s (" % func.name
        indent = " " * len(func_name)
        file.Write("%svoid* object" % func_name)
        file.Write(func.MakeTypedOriginalArgString(indent, separator = ",\n", add_separator = True), split=False)
        file.Write(")\n")
        file.Write("{\n")

        #file.Write("    INSTRUMENT ();")

        file.Write("    ")
        if func.return_type != "void":
            file.Write("return ")
        file.Write("real_%s (" % func.name)
        file.Write(func.MakeOriginalArgString(""))
        file.Write(");\n")
        file.Write("}\n\n")

    file.Write("void\n")
    file.Write("dispatch_table_fill_base (dispatch_table_t *dispatch)\n")
    file.Write("{\n")
    file.Write("    FunctionPointerType *temp = NULL;\n")

    for func in self.functions:
        file.Write('    dispatch->%s = passthrough_%s;\n' % (func.name, func.name))

    file.Write("temp = (FunctionPointerType *) &real_eglGetProcAddress;\n")
    file.Write('*temp = dlsym (libegl_handle (), "eglGetProcAddress");');

    for func in self.functions:
        if (func.name == "eglGetProcAddress"):
            continue
        file.Write('    temp = (FunctionPointerType *) &real_%s;\n' % func.name)
        file.Write('    *temp = ')
        file.Write('            find_gl_symbol (lib%s_handle (),' %(func.name.startswith('egl') and 'egl' or 'gl' ))
        file.Write('            real_eglGetProcAddress, "%s");\n' % func.name)

    file.Write("}\n")
    file.Close()

  def WriteGLGetType(self, filename):
    """Writes the glGet* functions for the client-side"""

    file = CWriter(filename)

    for func in _GL_GET_TYPE_INFO_FUNC:
        func_info = _GL_GET_TYPE_INFO_FUNC[func]
        file.Write("static void\n")
        file.Write("caching_client_%s (void* client, GLenum pname, %s *params)\n" % (func, func_info['type']))
        file.Write("{\n")
        file.Write("    INSTRUMENT();\n\n");

        file.Write("    egl_state_t *state = client_get_current_state (CLIENT (client));\n")
        file.Write("    if (! state)\n")
        file.Write("        return;\n\n")

        file.Write("    switch (pname) {\n")
        for enum_name in _GL_GET_TYPE_INFO:
            file.Write("    case %s:\n" % enum_name)
            info = _GL_GET_TYPE_INFO[enum_name]
            if 'size' in info:
                for i in range(info['size']):
                    file.Write("        params[%s] = state->%s[%s];\n" % (i, info['var'], i))

            elif 'index' in info:
                file.Write("       *params = state->%s[%s];\n" % (info['var'], info['index']))

            elif 'has_cache' in info:
                if not func_info['type'] == "GLboolean":
                    file.Write("       if (! state->%s_queried) {\n" % info['var'])
                    file.Write("           CACHING_CLIENT(client)->super_dispatch.%s (client, pname, params);\n" % func)
                    file.Write("           state->%s = *params;\n" % info['var'])
                    file.Write("           state->%s_queried = true;\n" % info['var'])
                    file.Write("       } else \n")
                file.Write("           *params = state->%s;\n" % info['var'])

            elif 'fetch_server_data' in info:
                file.Write("       CACHING_CLIENT(client)->super_dispatch.%s (client, pname, params);\n" % func)
                file.Write("       state->%s = *params;\n" % info['var'])

            else:
                file.Write("       *params = state->%s;\n" % info['var'])

            file.Write("        break;\n")

        file.Write("    default:\n")
        file.Write("        CACHING_CLIENT(client)->super_dispatch.%s (client, pname, params);\n" % func)
        file.Write("        break;\n")
        file.Write("    }\n")

        file.Write("    if (pname == GL_ARRAY_BUFFER_BINDING) {\n")
        file.Write("        vertex_attrib_list_t *attrib_list = &state->vertex_attribs;\n")
        file.Write("        vertex_attrib_t *attribs = attrib_list->attribs;\n")
        file.Write("        int count = attrib_list->count;\n")

        file.Write("        int i;\n")
        file.Write("        for (i = 0; i < count; i++) {\n")
        file.Write("            attribs[i].array_buffer_binding = *params;\n")
        file.Write("        }\n")
        file.Write("    }\n")
        file.Write("}\n\n")

    file.Close()

  def WriteCachingClientDispatchTableImplementation(self, filename):
    caching_client_text = open(os.path.join('..', 'client', 'caching_client.c')).read()
    file = CWriter(filename)

    for func in self.functions:
        caching_func_name = "caching_client_%s " % func.name
        if caching_client_text.find(caching_func_name) == -1 and func.name not in _GL_GET_TYPE_INFO_FUNC:
            continue
        file.Write('    client->super.dispatch.%s = %s;\n' % (func.name, caching_func_name))
    file.Close()

  def WriteCommandEnum(self, filename):
    """Writes the command format"""
    file = CWriter(filename)
    for func in self.functions:
      func.WriteEnumName(file)
    file.Write("\n")
    file.Close()

  def WriteEnumValidation(self, filename):
    """Writes the implementation for enum validation"""
    file = CWriter(filename)
    file.Write("#include \"config.h\"\n")
    file.Write("#include \"gl2ext.h\"\n")
    file.Write("#include <GLES2/gl2.h>\n\n")

    for (key, value) in _ENUM_LISTS.iteritems():
        file.Write("private bool\n")
        file.Write("is_valid_%s (%s value)\n" % (key, value['type']))
        file.Write("{\n")
        file.Write("    return %s;\n" %
            " ||\n        ".join([("value == %s" % type) for type in value['valid']]))
        file.Write("}\n\n")
    file.Close()

def main(argv):
  """This is the main function."""
  parser = OptionParser()
  parser.add_option(
      "-g", "--generate-implementation-templates", action="store_true",
      help="generates files that are generally hand edited..")
  parser.add_option(
      "--output-dir",
      help="base directory for resulting files, under chrome/src. default is "
      "empty. Use this if you want the result stored under gen.")
  parser.add_option(
      "-v", "--verbose", action="store_true",
      help="prints more output.")

  (options, args) = parser.parse_args(args=argv)

  script_dir = os.path.dirname(__file__)
  if script_dir:
      os.chdir(script_dir)

  gen = GLGenerator(options.verbose)
  gen.ParseAPIFiles()

  # Support generating files under gen/
  if options.output_dir != None:
    os.chdir(options.output_dir)

  # Shared between the client and the server.
  gen.WriteDispatchTable("dispatch_table_autogen.h")
  gen.WritePassthroughDispatchTableImplementation("dispatch_table_autogen.c")
  gen.WriteCommandHeader("command_autogen.h")
  gen.WriteCommandEnum("command_types_autogen.h")

  gen.WriteEnumValidation("enum_validation.h")

  # These are used on the client-side.
  gen.WriteCommandInitilizationAndSizeFunction("command_autogen.c")
  gen.WriteClientEntryPoints("client_entry_points.c")
  gen.WriteBaseClient("client_autogen.c")
  gen.WriteGLGetType("caching_client_glget.c")
  gen.WriteCachingClientDispatchTableImplementation("caching_client_dispatch_autogen.c")

  # These are used on the server-side.
  gen.WriteBaseServer("server_autogen.c")

  if gen.errors > 0:
    print "%d errors" % gen.errors
    return 1
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
