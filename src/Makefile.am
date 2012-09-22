BUILT_SOURCES =

lib_LTLIBRARIES = libGPUProcess.la
libGPUProcess_la_CFLAGS = \
	$(GLES_CFLAGS) \
	$(EGL_CFLAGS) \
	$(GL_CFLAGS) \
	-Igenerated \
	-Iclient \
	-Iserver \
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
	compiler_private.h \
	name_handler.h \
	name_handler.c \
	ring_buffer.h \
	ring_buffer.c \
	server/server.c \
	server/server.h \
	server/dispatch_private.h \
	server/gl_server_private.h \
	server/gl_states.h \
	thread_private.h \
	types_private.h

if HAS_GL
libGPUProcess_la_SOURCES += \
	server/dispatch_desktop.c \
	server/dispatch_private_desktop.h \
	glx_states.h
endif

if HAS_GLES2
libGPUProcess_la_SOURCES += \
	client/egl_api.c \
	client/gles2_api.c \
	client/gles2_api_private.h \
	egl_states.h \
	gles2_states.h \
	gles2_utils.c \
	gles2_utils.h \
	server/dispatch_gles2.c \
	server/egl_server.c \
	server/egl_server_private.h \
	server/egl_server_helper.c \
	server/gles2_server.c

nodist_libGPUProcess_la_SOURCES = \
	generated/dispatch_gles2_private.h \
	generated/gles2_api_autogen.c \
	generated/command_autogen.c \
	generated/command_autogen.h \
	generated/command_id_autogen.h

BUILT_SOURCES += \
	$(nodist_libGPUProcess_la_SOURCES)

generated/dispatch_gles2_private.h generated/gles2_api_autogen.c generated/command_autogen.c generated/command_autogen.h generated/command_id_autogen.h: generated/build_gles2_cmd_buffer.py generated/egl_functions.txt generated/gles2_functions.txt
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
