noinst_PROGRAMS = \
	server_test

#FIXME: remove this workaround
rootsrcdir=../..

server_test_SOURCES = \
	$(rootsrcdir)/src/ring_buffer.c \
	$(rootsrcdir)/src/ring_buffer.h \
	$(rootsrcdir)/src/server/caching_server_gles_tests.h \
	$(rootsrcdir)/src/server/caching_server_gles_private.h \
	$(rootsrcdir)/src/server/caching_server_gles.c \
	$(rootsrcdir)/src/server/server_dispatch_table.c \
	$(rootsrcdir)/src/server/server_dispatch_table.h \
	$(rootsrcdir)/src/server/server.c \
	$(rootsrcdir)/src/server/server.h \
	$(rootsrcdir)/src/name_handler.h \
	$(rootsrcdir)/src/name_handler.c \
	$(rootsrcdir)/src/command.h \
	$(rootsrcdir)/src/command.c \
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
	-I$(rootsrcdir)/src/generated \
	-I$(rootsrcdir)/src/server \
	-I$(rootsrcdir)/src/client \
	-Wall \
	-Werror
