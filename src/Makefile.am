BUILT_SOURCES =

lib_LTLIBRARIES = libGPUProcess.la
libGPUProcess_la_CFLAGS = \
	-Igenerated \
	-Iclient \
	-Werror

libGPUProcess_la_LDFLAGS = \
	-version-info 0:0:0 \
	-shared \
	-ldl \
	-lpthread \
	-no-undefined

libGPUProcess_la_SOURCES = \
	client/client_state.c \
	client/client_state.h \
	command.c \
	command.h \
	command_buffer.h \
	command_buffer.c \
	command_buffer_server.h \
	command_buffer_server.c \
	compiler_private.h \
	dispatch_private.h \
	dispatch.c \
	gl_server_private.h \
	gl_states.h \
	name_handler.h \
	name_handler.c \
	ring_buffer.h \
	ring_buffer.c \
	thread_private.h \
	types_private.h

nodist_libGPUProcess_la_SOURCES = \
	generated/gles2_api_autogen.c \
	generated/command_autogen.c \
	generated/command_autogen.h \
	generated/command_id_autogen.h

if HAS_GL
libGPUProcess_la_SOURCES += \
	glx_states.h
endif

if HAS_GLES2
libGPUProcess_la_SOURCES += \
	client/egl_api.c \
	client/gles2_api.c \
	client/gles2_api_private.h \
	egl_server.c \
	egl_server_private.h \
	egl_server_helper.c \
	egl_states.h \
	gles2_server.c \
	gles2_states.h \
	gles2_utils.c \
	gles2_utils.h

BUILT_SOURCES += \
	$(nodist_libGPUProcess_la_SOURCES)

generated/gles2_api_autogen.c generated/command_autogen.c: generated/build_gles2_cmd_buffer.py generated/gles2_functions.txt
	python generated/build_gles2_cmd_buffer.py
endif

library_dir = $(top_builddir)/libs
$(library_dir)/libGPUProcess.so: libGPUProcess.la
	$(AM_V_at) $(MKDIR_P) $(library_dir)
	$(AM_V_GEN) $(LN_S) -f ../src/.libs/libGPUProcess.so.0.0.0 $(library_dir)/libGPUProcess.so
symlinks = $(library_dir)/libGPUProcess.so

if HAS_GLES2
$(library_dir)/libEGL.so:
	$(AM_V_GEN) $(LN_S) -f ../src/.libs/libGPUProcess.so $(library_dir)/libEGL.so
$(library_dir)/libGLESv2.so:
	$(AM_V_GEN) $(LN_S) -f ../src/.libs/libGPUProcess.so $(library_dir)/libGLESv2.so
symlinks += $(library_dir)/libEGL.so $(library_dir)/libGLESv2.so
endif

if HAS_GL
$(library_dir)/libGL.so:
	ln -f ../src/.libs/libGPUProcess.so $(library_dir)/libGL.so
symlinks += $(library_dir)/libGL.so
endif

all-local: $(symlinks)
clean-local:
	rm generated/*.c generated/*.h
