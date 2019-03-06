###############################################
#xminerd
###############################################
#ACLOCAL_AMFLAGS = -I m4
#get static lib

EXTRA_CFLAGS= -I$(srcdir)/include -Iinclude/qsdk/ $(LIBMINER_CFLAGS) 

bin_PROGRAMS = xminerd
xminerd_SOURCES =  fan.c \
                   ping.c \
                   base.c \
                   xminerd.c

xminerd_LDFLAGS= -version-info 1:0:0 $(LIBMINER_LIBS) -ljansson
xminerd_CFLAGS= $(EXTRA_CFLAGS)

