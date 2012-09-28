noinst_PROGRAMS = \
	client_test

#FIXME: remove this workaround
rootsrcdir=../..

client_test_SOURCES = \
	$(rootsrcdir)/src/name_handler.h \
	$(rootsrcdir)/src/name_handler.c \
	$(rootsrcdir)/src/command.h \
	$(rootsrcdir)/src/command.c \
	$(rootsrcdir)/src/client/client.h \
	$(rootsrcdir)/src/client/client.c \
	$(rootsrcdir)/src/generated/command_autogen.c \
	$(rootsrcdir)/src/ring_buffer.c \
	$(rootsrcdir)/src/ring_buffer.h \
	$(rootsrcdir)/src/server/server_dispatch_table.c \
	$(rootsrcdir)/src/server/server_dispatch_table.h \
	$(rootsrcdir)/src/server/server.c \
	$(rootsrcdir)/src/server/server.h \
	main.c

client_test_LDFLAGS = \
	-lX11 \
	-ldl \
	-lpthread

client_test_CFLAGS = \
	-g \
	-DENABLE_TESTING=1 \
	$(GLES_CFLAGS) \
	$(EGL_CFLAGS) \
	$(GL_CFLAGS) \
	-I$(rootsrcdir)/src \
	-I$(rootsrcdir)/src/client \
	-I$(rootsrcdir)/src/generated \
	-I$(rootsrcdir)/src/server \
	-Wall \
	-Werror
