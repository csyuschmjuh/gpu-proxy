noinst_PROGRAMS = \
	gpuproxy_test

#FIXME: remove this workaround
rootsrcdir=../..

gpuproxy_test_SOURCES = \
	$(rootsrcdir)/src/dispatch_private.h \
	$(rootsrcdir)/src/dispatch.c \
	$(rootsrcdir)/src/egl_states.h \
	$(rootsrcdir)/src/egl_server_private.h \
	$(rootsrcdir)/src/egl_server.c \
	$(rootsrcdir)/src/egl_server_helper.c \
	$(rootsrcdir)/src/gles2_server.c \
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
	-Werror
