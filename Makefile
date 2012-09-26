CC ?= gcc

# Use the first CFLAGS for a highly optimized version, or the second for a
# debugable version
#CFLAGS = -O6 -funroll-loops -fexpensive-optimizations -m486 -Wall
#CFLAGS = -g3 -Wall

# If the following is uncommented, debugging information will be displayed
# while nwreckdum is working
#CFLAGS += -DDEBUG

# If you are trying to compile this for DOS, the following line must be
# uncommented to provide a workaround for the lack of snprintf()
#CFLAGS += -DDOS

OBJ = nwreckdum.o shhopt.o

all: ${OBJ} nwreckdum

#.c.o: debdb.h
#	${CC} ${CFLAGS} -o $@ -c $<

nwreckdum: ${OBJ}
	${CC} ${CFLAGS} -o nwreckdum ${OBJ}

clean:
	rm -f ${OBJ}
	rm -f nwreckdum
