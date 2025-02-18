default:
	gcc -std=c99  -o main.exe main.c -lSDL3 -lSDL3_ttf
run:
	main.exe

test:
	gcc -std=c99 -o test.exe test.c -lSDL3
