#Create lib_simpleloader.so from loader.c
all:
	gcc -c -m32 -fpic loader.c
	gcc -m32 -shared -o ../bin/lib_simpleloader.so loader.o
clean:
	-@rm  loader.o
