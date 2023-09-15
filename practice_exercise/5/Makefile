jsim : joinsim.o bufpool.o
	gcc -o jsim joinsim.o bufpool.o

joinsim.o : joinsim.c bufpool.h
	gcc -c joinsim.c

bufpool.o : bufpool.c bufpool.h
	gcc -c bufpool.c

clean :
	rm -f jsim *.o core
