CC=gcc
CFLAGS=-c -O2
LDFLAGS=
CSOURCES=quickerd.c
COBJECTS=$(CSOURCES:.c=.o)
EXECUTABLE=quickerd

ifeq ($(OS),Windows_NT)
		CC=mingw32-gcc
		EXECUTABLE=quickerd.exe
		CFLAGS=$(CFLAGS) -I pcre/include
		LDFLAGS=-L pcre/lib -lpcre
endif

all : $(EXECUTABLE)
debug : CFLAGS=-c -ggdb

ifeq ($(OS),Windows_NT)
		debug : CFLAGS=$(CFLAGS) -I pcre/include
		debug : LDFLAGS=-L pcre/lib -lpcre
endif

debug : $(EXECUTABLE)

$(EXECUTABLE) : $(COBJECTS) 
		$(CC) $(LDFLAGS) $^ -o $@

.c.o :
		$(CC) $(CFLAGS) $< -o $@

clean :
	rm $(COBJECTS) $(EXECUTABLE)
