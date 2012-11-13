noinst_PROGRAMS = \
	server_test

#FIXME: remove this workaround
rootsrcdir=../..

server_test_SOURCES = \
	$(rootsrcdir)/src/compiler.c \
	$(rootsrcdir)/src/client/client.c \
	$(rootsrcdir)/src/client/client.h \
	$(rootsrcdir)/src/client/caching_client_private.h \
	$(rootsrcdir)/src/client/caching_client.c \
	$(rootsrcdir)/src/client/caching_client.h \
	$(rootsrcdir)/src/client/egl_api_custom.c \
	$(rootsrcdir)/src/client/name_handler.h \
	$(rootsrcdir)/src/client/name_handler.c \
	$(rootsrcdir)/src/dispatch_table.c \
	$(rootsrcdir)/src/dispatch_table.h \
	$(rootsrcdir)/src/egl_state.c \
	$(rootsrcdir)/src/egl_state.h \
	$(rootsrcdir)/src/gles2_state.c \
	$(rootsrcdir)/src/gles2_state.h \
	$(rootsrcdir)/src/types_private.h \
	$(rootsrcdir)/src/types_private.c \
	$(rootsrcdir)/src/util/gles2_utils.c \
	$(rootsrcdir)/src/util/gles2_utils.h \
	$(rootsrcdir)/src/util/hash.c \
	$(rootsrcdir)/src/util/hash.h \
	$(rootsrcdir)/src/ring_buffer.c \
	$(rootsrcdir)/src/ring_buffer.h \
	$(rootsrcdir)/src/server/server.c \
	$(rootsrcdir)/src/server/server.h \
	$(rootsrcdir)/src/server/server_custom.c \
	$(rootsrcdir)/src/server/server_custom.h \
	$(rootsrcdir)/src/command.c \
	$(rootsrcdir)/src/command.h \
	$(rootsrcdir)/src/command_custom.c \
	$(rootsrcdir)/src/generated/command_autogen.c \
	gpuprocess_test.h \
	gpuprocess_test.c \
	test_egl.c \
	test_egl2.h \
	test_egl2.c \
	test_gles.h \
	test_gles.c \
	main.c

server_test_LDFLAGS = \
	-lX11 \
	-ldl \
	-lpthread

server_test_CFLAGS = \
	-g \
	-DENABLE_TESTING=1 \
	$(GLES_CFLAGS) \
	$(EGL_CFLAGS) \
	$(GL_CFLAGS) \
	-I$(rootsrcdir)/src \
	-I$(rootsrcdir)/src/client \
	-I$(rootsrcdir)/src/generated \
	-I$(rootsrcdir)/src/server \
	-I$(rootsrcdir)/src/util \
        -Werror \
	-Wall
