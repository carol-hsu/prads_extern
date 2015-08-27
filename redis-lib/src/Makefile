# redis library Makefile

# source files.
SRC =   redis_lib.c	
 
OBJ = $(SRC:.c=.o)
 
OUT = ../libredis.a
 
# include directories
INCLUDES = -I. -I../deps/hiredis -I/usr/include -I/usr/include/linux -I/usr/include/c++/4.8/tr1/ -I/usr/include/c++/4.8/
 
# C++ compiler flags (-g -O2 -Wall)
CCFLAGS = -g
 
# compiler
CCC = gcc
 
# library paths
LIBS = -L../ -L/usr/local/lib ../deps/hiredis/libhiredis.a
 
# compile flags
LDFLAGS = -g
 
.SUFFIXES: .c

all: default
 
default: dep $(OUT)
 
.cpp.o:
	$(CCC) $(INCLUDES) $(CCFLAGS) -c $< -o $@
 
$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)
 
depend: dep
 
dep:
	makedepend -- $(CFLAGS) -- $(INCLUDES) $(SRC)
 
clean:
	rm -f $(OBJ) $(OUT) Makefile.bak
# DO NOT DELETE

redis_lib.o: redis_lib.h ../deps/hiredis/hiredis.h /usr/include/stdio.h
redis_lib.o: /usr/include/features.h /usr/include/stdc-predef.h
redis_lib.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
redis_lib.o: /usr/include/gnu/stubs.h /usr/include/linux/stddef.h
redis_lib.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
redis_lib.o: /usr/include/libio.h /usr/include/_G_config.h
redis_lib.o: /usr/include/wchar.h /usr/include/c++/4.8/tr1/stdarg.h
redis_lib.o: /usr/include/c++/4.8/tr1/cstdarg
redis_lib.o: /usr/include/c++/4.8/tr1/cstdarg /usr/include/bits/stdio_lim.h
redis_lib.o: /usr/include/bits/sys_errlist.h /usr/include/sys/time.h
redis_lib.o: /usr/include/time.h /usr/include/bits/time.h
redis_lib.o: /usr/include/sys/select.h /usr/include/bits/select.h
redis_lib.o: /usr/include/bits/sigset.h /usr/include/string.h
redis_lib.o: /usr/include/xlocale.h
