CC = gcc
LIBS = `pkg-config --libs gtk+-3.0`
INCS = `pkg-config --cflags gtk+-3.0`
#LIBS = `gtk-config --libs`
#INCS = `gtk-config --cflags`

FLAGS = -Wall -g

main: gexec
gexec: autocomp.o gexec.o
	$(CC) autocomp.o gexec.o -o gexec $(LIBS) $(INCS) $(FLAGS)
	strip gexec
gexec.o: gexec.c
	$(CC) -c gexec.c $(INCS) $(FLAGS)
autocomp.o: autocomp.c autocomp.h
	$(CC) -c autocomp.c $(INCS) $(FLAGS)
clean:
	-rm gexec
	-rm *.o
install:
	-cp gexec /usr/local/bin
	-cp gexec.1 /usr/local/man/man1/
