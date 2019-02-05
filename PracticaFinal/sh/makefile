OBJS = array.o str.o shell.o
BYNARY = shell

CFLAGS = -Wall -Wshadow -g

shell: $(OBJS)
	gcc -o $(BYNARY) $(OBJS)

shell.o: shell.c
	gcc -c $(CFLAGS) shell.c  

str.o: str.c str.h
	gcc -c $(CFLAGS) str.c

array.o: array.c array.h
	gcc -c $(CFLAGS) array.c

clean:
	rm -f $(BYNARY) $(OBJS)
