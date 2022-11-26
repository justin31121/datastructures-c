all:
	gcc -DSERVE_SDL main.c -lmingw32 -lSDL2main -lSDL2 -lm -Wall -Wextra -Werror -o main
