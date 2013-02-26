BUILT_SOURCES =

lib_LTLIBRARIES = libGPUProcess.la
libGPUProcess_la_CFLAGS = \
	$(GLES_CFLAGS) \
	-Werror \
	-Wall \
	-Igenerated \
	-Iclient \
	-Iserver \
	-Iutil

libGPUProcess_la_LDFLAGS = \
	-version-info 0:0:0 \
	-shared \
	-ldl \
	-lpthread \
	-no-undefined

libGPUProcess_la_SOURCES = \
	client/client.c \
	client/client.h \
	client/name_handler.h \
	client/name_handler.c \
	command.c \
	command.h \
	command_custom.c \
	command_custom.h \
	compiler_private.h \
	compiler.c \
	gl_states.h \
	ring_buffer.h \
	ring_buffer.c \
	server/gl_server_private.h \
	server/server.h \
	server/server.c \
	thread_private.h \
	types_private.h \
	types_private.c \
	util/hash.h \
	util/hash.c

libGPUProcess_la_SOURCES += \
	client/egl_api_custom.c \
	client/caching_client.c \
	client/caching_client.h \
	client/caching_client_private.h \
	dispatch_table.c \
	dispatch_table.h \
	egl_state.c \
	egl_state.h \
	program.c \
	program.h \
	util/gles2_utils.c \
	util/gles2_utils.h


nodist_libGPUProcess_la_SOURCES = \
	generated/client_entry_points.c \
	generated/command_autogen.c \
	generated/command_autogen.h \
	generated/command_types_autogen.h

BUILT_SOURCES += \
	$(nodist_libGPUProcess_la_SOURCES)

generated/command_autogen.c: generated/command_autogen.h
generated/command_autogen.h: generated/command_types_autogen.h
generated/command_types_autogen.h: generated/client_entry_points.c
generated/client_entry_points.c: generated/client_autogen.c
generated/client_autogen.c: generated/server_autogen.c
generated/server_autogen.c: generated/dispatch_table_autogen.c
generated/dispatch_table_autogen.c: generated/dispatch_table.h
generated/dispatch_table.h: generated/build_gles2_cmd_buffer.py \
								   generated/egl_functions.txt \
								   generated/gles2_functions.txt
	python generated/build_gles2_cmd_buffer.py

library_dir = $(top_builddir)/libs
$(library_dir)/libGPUProcess.so: libGPUProcess.la
	$(AM_V_at) $(MKDIR_P) $(library_dir)
	$(AM_V_GEN) $(LN_S) -f ../src/.libs/libGPUProcess.so.0.0.0 $(library_dir)/libGPUProcess.so
symlinks = $(library_dir)/libGPUProcess.so

$(library_dir)/libEGL.so:
	$(AM_V_GEN) $(LN_S) -f ../src/.libs/libGPUProcess.so $(library_dir)/libEGL.so
$(library_dir)/libGLESv2.so:
	$(AM_V_GEN) $(LN_S) -f ../src/.libs/libGPUProcess.so $(library_dir)/libGLESv2.so
symlinks += $(library_dir)/libEGL.so $(library_dir)/libGLESv2.so

all-local: $(symlinks)
clean-local:
	rm -f generated/*.c generated/*.h
