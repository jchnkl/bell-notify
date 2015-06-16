LIBS+=$(shell pkg-config --libs libcanberra) -lutil
CFLAGS+=$(shell pkg-config --cflags libcanberra) -g -std=gnu99

all: bell-notify

bell-notify: bell-notify.c
	gcc ${CFLAGS} ${LIBS} $@.c -o $@
