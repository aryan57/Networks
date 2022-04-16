main: user1 user2
user1: user1.o librsocket.a
	gcc -o user1 user1.o -L. -lrsocket -lpthread
user2: user2.o librsocket.a 
	gcc -o user2 user2.o -L. -lrsocket -lpthread
user1.o: user1.c rsocket.h
	gcc -Wall -c user1.c -lpthread
user2.o: user2.c rsocket.h
	gcc -Wall -c user2.c -lpthread
librsocket.a: rsocket.o
	ar -rcs librsocket.a rsocket.o
rsocket.o: rsocket.c rsocket.h
	gcc -c rsocket.c -lpthread
clean:
	rm -f user1 user2 *.a *.o
