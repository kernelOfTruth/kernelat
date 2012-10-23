PROG = kernelat-spawner
OBJ = kernelat-spawner.o mm.o
PREFIX ?= /usr/local
CC ?= cc
CFLAGS = -O3 -std=c99 -W -Wall -pedantic -D_GNU_SOURCE
LDADD = -lpthread -lm

all: build

build: $(PROG)

$(PROG): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDADD) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $<

install:
	install -Dm 0755 $(PROG) $(DESTDIR)$(PREFIX)/bin/$(PROG)

clean:
	rm -f $(OBJ) $(PROG)

