all: useLib

useLib: main.o libsearchFile.a
	gcc -static -lm -o useLib main.o -L. -lsearchFile

main.o: main.c
	gcc -O -c main.c

addSorted.o: searchFile.c searchFile.h
	gcc -O -c searchFile.c
		
libsearchFile.a: searchFile.o
	ar rcs libsearchFile.a searchFile.o

libs: searchFile.a

clean:
	rm -f useLib *.o *.a *.gch