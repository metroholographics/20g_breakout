default:
	gcc -std=c99  -o main.exe main.c -lSDL3 
run:
	main.exe

test:
	gcc -std=c99 -o test.exe test.c -lSDL3
