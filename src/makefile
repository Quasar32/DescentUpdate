CPPFLAGS = -Wall -g -O3
OBJFILES = main.o descent.o error.o frame.o worker.o render.o tile_data.o

output: $(OBJFILES) 
	gcc *.o -mconsole -mwindows -lwinmm -o ../build/descent 

main.o: main.c descent.h error.h
	gcc -c main.c $(CPPFLAGS)	

descent.o: descent.c descent.h vec2.h worker.h render.h tile_data.c
	gcc -c descent.c $(CPPFLAGS) 

error.o: error.c error.h
	gcc -c error.c $(CPPFLAGS) 

frame.o: frame.c frame.h
	gcc -c frame.c $(CPPFLAGS)

worker.o: worker.c worker.h
	gcc -c worker.c $(CPPFLAGS)

render.o: render.c render.h tile_data.h
	gcc -c render.c $(CPPFLAGS)

tile_data.o: tile_data.h tile_data.c
	gcc -c tile_data.c $(CPPFLAGS)

clean:
	del *.o
