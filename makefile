CFLAGS = -Wall 

output: 
	gcc src/win32_descent.c -mwindows -o build/descent $(CFLAGS) 
	gcc src/descent.c -shared -o build/descent.dll $(CFLAGS) 

