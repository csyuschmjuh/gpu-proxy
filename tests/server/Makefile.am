noinst_PROGRAMS = \
	gpuproxy_test

#FIXME: remove this workaround
rootsrcdir=../..

gpuproxy_test_SOURCES = \
	$(rootsrcdir)/src/dispatch.h \
	$(rootsrcdir)/src/dispatch.c \
	$(rootsrcdir)/src/egl_states.h \
	$(rootsrcdir)/src/egl_server_private.h \
	$(rootsrcdir)/src/egl_server.c \
	$(rootsrcdir)/src/egl_server_helper.c \
	gpuprocess_test.h \
	gpuprocess_test.c \
	tst_egl.c \
	tst_egl2.h \
	tst_egl2.c \
	main.c

gpuproxy_test_LDFLAGS = \
	-lX11 \
	-ldl \
	-lpthread

gpuproxy_test_CFLAGS = \
	-DENABLE_TESTING=1 \
	-I$(rootsrcdir)/src \
	-Werror
