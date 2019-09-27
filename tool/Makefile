miscgen : miscgen.o
	gcc -o miscgen miscgen.o -lz

miscgen.o : miscgen.c
	gcc -c miscgen.c -I../ -I../../../../../../

clean :
	rm miscgen.o miscgen
