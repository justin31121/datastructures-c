CFLAGS = -Wall -Wextra -pedantic -Wshadow
SDL_CURL = -lmingw32 -lSDL2main -lSDL2 -lcurl

.all:
ifeq ($(OS),Windows_NT)
	gcc main.c $(CFLAGS) -lssl -lcrypto -o main -lws2_32 -lpthread
else
	gcc main.c $(CFLAGS) -lssl -lcrypto -o main -lpthread
endif

main:
	gcc main.c $(CFLAGS) -lssl -lcrypto -o main -lws2_32 -lpthread

get_demo:
	gcc get_demo.c $(CFLAGS) -lssl -lcrypto -o get_demo -lws2_32

array_demo:
	gcc array_demo.c $(CFLAGS) -o array_demo

hashtable_demo:
	gcc hashtable_demo.c $(CFLAGS)-o hashtable_demo

json_demo:
	gcc json_demo.c $(CFLAGS) -o json_demo

json_parse_demo:
	gcc json_parse_demo.c $(CFLAGS) -o json_parse_demo

sdl_curl_json_demo:
	gcc sdl_curl_json_demo.c $(CFLAGS) $(SDL_CURL) -o sdl_curl_json_demo
