CFLAGS = -Wall -Wextra -pedantic -Wshadow
SDL_CURL = -lmingw32 -lSDL2main -lSDL2 -lcurl

ifeq ($(OS),Windows_NT)
	LDFLAGS = -lws2_32 -lpthread -lssl -lcrypto
else
	LDFLAGS = -lpthread -lssl -lcrypto
endif

all: ./src/main.c ./libs/http.h ./libs/util.h ./libs/string.h
	gcc ./src/main.c $(CFLAGS) -ggdb -o main $(LDFLAGS)

gui: ./src/gui_demo.c ./libs/gui.h
	gcc ./src/gui_demo.c $(CFLAGS) -ggdb -o gui -lgdi32 -lopengl32

google: ./src/google.c ./libs/http.h ./libs/http.h ./libs/util.h ./libs/string.h ./libs/tokenizer.h ./libs/html_parser.h
	gcc ./src/google.c $(CFLAGS) -ggdb -o google $(LDFLAGS)

email: ./src/email.c ./libs/http.h
	gcc ./src/email.c $(CFLAGS) -o email $(LDFLAGS)

encode: ./src/encode.c ./libs/http.h
	gcc ./src/encode.c $(CFLAGS) -ggdb -o encode -lpthread

httpliveserver: ./src/httpserver.c ./libs/http.h ./libs/util.h ./libs/string.h
	gcc ./src/httpliveserver.c $(CFLAGS) -ggdb -pthread -o httpliveserver -lws2_32

httpserver: ./src/httpserver.c ./libs/http.h ./libs/util.h ./libs/string.h
	gcc ./src/httpserver.c $(CFLAGS) -ggdb $(LDFLAGS) -o httpserver

httpcat: ./src/httpcat.c ./libs/http.h ./libs/util.h ./libs/string.h
	gcc ./src/httpcat.c $(CFLAGS) -ggdb $(LDFLAGS) -o httpcat

watcher_demo:
	gcc ./src/watcher_demo.c $(CFLAGS) -o watcher_demo $(LDFLAGS)

get_demo: ./src/get_demo.c ./libs/http.h
	gcc ./src/get_demo.c $(CFLAGS) -o get_demo $(LDFLAGS)

array_demo:
	gcc ./src/array_demo.c $(CFLAGS) -o array_demo

hashtable_demo:
	gcc ./src/hashtable_demo.c $(CFLAGS)-o hashtable_demo

json_demo:
	gcc ./src/json_demo.c $(CFLAGS) -o json_demo

json_parse_demo:
	gcc ./src/json_parse_demo.c $(CFLAGS) -o json_parse_demo

sdl_curl_json_demo:
	gcc ./src/sdl_curl_json_demo.c $(CFLAGS) $(SDL_CURL) -o sdl_curl_json_demo

luigi_demo: ./src/luigi_demo.c
	gcc ./src/luigi_demo.c -Wextra -o luigi_demo -lGdi32
