all:
ifeq ($(OS),Windows_NT)
	gcc main.c -Wall -Wextra -pedantic -lssl -lcrypto -o main -lws2_32 -lpthread
else
	gcc main.c -Wall -Wextra -pedantic -lssl -lcrypto -o main -lpthread
endif

