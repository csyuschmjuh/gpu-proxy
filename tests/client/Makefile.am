noinst_PROGRAMS = \
	client_test

#FIXME: remove this workaround
rootsrcdir=../..

client_test_SOURCES = \
	$(rootsrcdir)/src/command.c \
	$(rootsrcdir)/src/command.h \
	$(rootsrcdir)/src/command_custom.c \
	$(rootsrcdir)/src/command_custom.h \
	$(rootsrcdir)/src/compiler.c \
	$(rootsrcdir)/src/dispatch_table.c \
	$(rootsrcdir)/src/dispatch_table.h \
	$(rootsrcdir)/src/client/client.c \
	$(rootsrcdir)/src/client/client.h \
	$(rootsrcdir)/src/client/caching_client.c \
	$(rootsrcdir)/src/client/caching_client.h \
	$(rootsrcdir)/src/client/egl_api_custom.c \
	$(rootsrcdir)/src/client/name_handler.c \
	$(rootsrcdir)/src/client/name_handler.h \
	$(rootsrcdir)/src/generated/command_autogen.c \
	$(rootsrcdir)/src/generated/client_entry_points.c \
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
	$(rootsrcdir)/tests/server/gpuprocess_test.c \
	$(rootsrcdir)/tests/server/gpuprocess_test.h \
	basic_test.c \
	basic_test.h \
	main.c

client_test_LDFLAGS = \
	-lX11 \
	-ldl \
	-lpthread

client_test_CFLAGS = \
	-g \
	-DENABLE_TESTING=1 \
	-rdynamic \
	$(GLES_CFLAGS) \
	$(EGL_CFLAGS) \
	$(GL_CFLAGS) \
	-I$(rootsrcdir)/src \
	-I$(rootsrcdir)/src/client \
	-I$(rootsrcdir)/src/generated \
	-I$(rootsrcdir)/src/server \
	-I$(rootsrcdir)/src/util \
	-I$(rootsrcdir)/tests/server \
	-Werror \
	-Wall
