CPPFLAGS = -Wall -g -O3
OBJFILES = audio.o descent.o error.o frame.o main.o procs.o render.o stb_vorbis.o tile_data.o worker.o
LINKFLAGS = -mconsole -mwindows

output: $(OBJFILES)
	gcc $(OBJFILES) -o ../build/descent $(LINKFLAGS)

audio.o: audio.c audio.h error.h procs.h stb_vorbis.h
	gcc -c audio.c $(CPPFLAGS)

descent.o: descent.c color.h descent.h render.h tile_data.h vec2.h worker.h
	gcc -c descent.c $(CPPFLAGS)

error.o: error.c error.h
	gcc -c error.c $(CPPFLAGS)

frame.o: frame.c frame.h procs.h
	gcc -c frame.c $(CPPFLAGS)

main.o: main.c audio.h color.h descent.h error.h frame.h procs.h stb_vorbis.h tile_data.h vec2.h worker.h
	gcc -c main.c $(CPPFLAGS)

procs.o: procs.c procs.h
	gcc -c procs.c $(CPPFLAGS)

render.o: render.c descent.h render.h scalar.h tile_data.h vec2.h
	gcc -c render.c $(CPPFLAGS)

stb_vorbis.o: stb_vorbis.c stb_vorbis.h
	gcc -c stb_vorbis.c $(CPPFLAGS)

tile_data.o: tile_data.c tile_data.h
	gcc -c tile_data.c $(CPPFLAGS)

worker.o: worker.c worker.h
	gcc -c worker.c $(CPPFLAGS)

clean: 
	del *.o
