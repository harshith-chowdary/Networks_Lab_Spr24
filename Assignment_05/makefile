lib: msocket.c msocket.h
	@echo "\n****Building library****"
	gcc -c -Wall msocket.c -o msocket.o -I. 
	ar rcs libmsocket.a msocket.o
	@echo "****Library built****\n"

fuser1: file_user1.c
	@echo "\n****Building file_user1****"
	gcc -c -Wall file_user1.c -o fuser1.o -I. 
	gcc fuser1.o -o fuser1 -L. -lmsocket
	@echo "****File_user1 built****\n"

fuser2: file_user2.c
	@echo "\n****Building file_user2****"
	gcc -c -Wall file_user2.c -o fuser2.o -I.
	gcc fuser2.o -o fuser2 -L. -lmsocket
	@echo "****File_user2 built****\n"

user1: user1.c
	@echo "\n****Building user1****"
	gcc -c -Wall user1.c -o user1.o -I. 
	gcc user1.o -o user1 -L. -lmsocket
	@echo "****User1 built****\n"

user2: user2.c
	@echo "\n****Building user2****"
	gcc -c -Wall user2.c -o user2.o -I.
	gcc user2.o -o user2 -L. -lmsocket
	@echo "****User2 built****\n"

init: initmsocket.c
	@echo "\n****Initializing****"
	gcc -g -pthread initmsocket.c -o initmsocket -L. -lmsocket -I.
	@echo "****Initialized****\n"
	./initmsocket

socket_test: socket_test.c
	@echo "\nTesting Socket calls"
	gcc -g socket_test.c -o socket_test -L. -lmsocket -I.

run: socket_test
	./socket_test

clear:
	@echo "\n****Clearing****"
	gcc clear.c -o clear1 -I.
	./clear1
	@echo "****Cleared****\n"

clean:
	@echo "\n****Cleaning up****"
	rm -f *.o  *.a user1 user2 initmsocket clear1 msocket socket_test
	@echo "****Cleaned****\n"
