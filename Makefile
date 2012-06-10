
polaroid: main.c pnm.o comm.o jpeg.o huffman.o
	gcc main.c pnm.o comm.o jpeg.o huffman.o -o polaroid -lm -Wall

pnm.o: pnm.c pnm.h
	gcc -c pnm.c -o pnm.o -Wall

comm.o: comm.c comm.h
	gcc -c comm.c -o comm.o -Wall

jpeg.o: jpeg.c jpeg.h
	gcc -c jpeg.c -o jpeg.o -Wall

huffman.o: huffman.c huffman.h
	gcc -c huffman.c -o huffman.o -Wall

.PHONY: clean
clean:
	rm -f *.o

