CFLAGS = -std=c2x -Wall -Wextra -Wpedantic -g

output: 
	gcc win32_descent.c -mwindows -o descent $(CFLAGS) 
	gcc descent.c -shared -o descent.dll $(CFLAGS)

