CC = gcc
CFLAGS = -Wall -g
INCLUDES = -I../ -I../../

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

LTOBJS = lltest.o 

lltest: $(LTOBJS)
	$(CC) -o lltest $(LTOBJS) ../ll.o

test: main.o
	gcc -o test main.o ../libtoc.a

clean:
	rm -rf lltest test main.o $(LTOBJS)
