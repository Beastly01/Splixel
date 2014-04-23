
CC=gcc
CFLAGS=-I.
DEPS = hellomake.h
LIBS =-lpng12

OBJ = lights.o driver.o  

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

driver: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)
