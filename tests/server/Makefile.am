noinst_PROGRAMS = \
	gpuproxy_test

#FIXME: remove this workaround
rootsrcdir=../..

gpuproxy_test_SOURCES = \
	$(rootsrcdir)/src/ring_buffer.c \
	$(rootsrcdir)/src/ring_buffer.h \
	$(rootsrcdir)/src/server/caching_server_gles_tests.h \
	$(rootsrcdir)/src/server/caching_server_gles_private.h \
	$(rootsrcdir)/src/server/caching_server_gles.c \
	$(rootsrcdir)/src/server/server.c \
	$(rootsrcdir)/src/server/server.h \
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
	-g \
	-DENABLE_TESTING=1 \
	-I$(rootsrcdir)/src \
	-I$(rootsrcdir)/src/generated \
	-I$(rootsrcdir)/src/server \
	-Wunused-function \
	-Werror
