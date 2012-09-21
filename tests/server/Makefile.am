noinst_PROGRAMS = \
	gpuproxy_test

#FIXME: remove this workaround
rootsrcdir=../..

gpuproxy_test_SOURCES = \
	$(rootsrcdir)/src/egl_states.h \
	$(rootsrcdir)/src/egl_server_private.h \
	$(rootsrcdir)/src/ring_buffer.c \
	$(rootsrcdir)/src/ring_buffer.h \
	$(rootsrcdir)/src/server/command_buffer_server.c \
	$(rootsrcdir)/src/server/command_buffer_server.h \
	$(rootsrcdir)/src/server/dispatch_private.h \
	$(rootsrcdir)/src/server/dispatch_gles2.c \
	$(rootsrcdir)/src/server/egl_server.c \
	$(rootsrcdir)/src/server/egl_server_helper.c \
	$(rootsrcdir)/src/server/gles2_server.c \
	gpuprocess_test.h \
	gpuprocess_test.c \
	test_egl.c \
	test_egl2.h \
	test_egl2.c \
	test_gles.h \
	test_gles.c \
	main.c

gpuproxy_test_LDFLAGS = \
	-lX11 \
	-ldl \
	-lpthread

gpuproxy_test_CFLAGS = \
	-DENABLE_TESTING=1 \
	-I$(rootsrcdir)/src \
	-I$(rootsrcdir)/src/generated \
	-I$(rootsrcdir)/src/server \
	-Werror
