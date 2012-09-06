BUILT_SOURCES =

lib_LTLIBRARIES = libGPUProcess.la
libGPUProcess_la_CFLAGS = \
	-Igenerated \
	-Werror

libGPUProcess_la_LDFLAGS = \
	-version-info 0:0:0 \
	-ldl

libGPUProcess_la_SOURCES = \
	gpuprocess_command.h \
	gpuprocess_command_buffer.h \
	gpuprocess_command_buffer.c \
	gpuprocess_command_buffer_server.h \
	gpuprocess_command_buffer_server.c \
	gpuprocess_compiler_private.h \
	gpuprocess_dispatch_private.h \
	gpuprocess_dispatch.h \
	gpuprocess_dispatch.c \
	gpuprocess_gl_server_private.h \
	gpuprocess_gl_states.h \
	gpuprocess_ring_buffer.h \
	gpuprocess_ring_buffer.c \
	gpuprocess_thread_private.h \
	gpuprocess_types_private.h

if HAS_GL
libGPUProcess_la_SOURCES += \
	gpuprocess_glx_states.h
endif

if HAS_GLES2
libGPUProcess_la_SOURCES += \
	gpuprocess_egl_api.c \
	gpuprocess_egl_server.c \
	gpuprocess_egl_server_private.h \
	gpuprocess_egl_server_helper.c \
	gpuprocess_egl_states.h \
	gpuprocess_gles2_api.c \
	gpuprocess_gles2_server.c \
	gpuprocess_gles2_states.h

nodist_libGPUProcess_la_SOURCES = \
	generated/gpuprocess_gles2_api_autogen.c \
	generated/gpuprocess_command_autogen.c \
	generated/gpuprocess_command_autogen.h \
	generated/gpuprocess_command_id_autogen.h

BUILT_SOURCES += \
	$(nodist_libGPUProcess_la_SOURCES)

generated/gpuprocess_gles2_api_autogen.c : generated/build_gles2_cmd_buffer.py generated/gles2_functions.txt
	python generated/build_gles2_cmd_buffer.py
endif

library_dir = $(top_builddir)/libs
$(library_dir)/libGPUProcess.so: libGPUProcess.la
	$(AM_V_at) $(MKDIR_P) $(library_dir)
	$(AM_V_GEN) $(LN_S) -f .libs/libGPUProcess.so.0.0.0 $(library_dir)/libGPUProcess.so
symlinks = $(library_dir)/libGPUProcess.so

if HAS_GLES2
$(library_dir)/libEGL.so:
	$(AM_V_GEN) $(LN_S) -f $(library_dir)/libGPUProcess.so $(library_dir)/libEGL.so
$(library_dir)/libGLESv2.so:
	$(AM_V_GEN) $(LN_S) -f $(library_dir)/libGPUProcess.so $(library_dir)/libGLESv2.so
symlinks += $(library_dir)/libEGL.so $(library_dir)/libGLESv2.so
endif

if HAS_GL
$(library_dir)/libGL.so:
	ln -f $(library_dir)/libGPUProcess.so $(library_dir)/libGL.so
symlinks += $(library_dir)/libGL.so
endif

all-local: $(symlinks)
clean-local:
	rm generated/*.c generated/*.h
