all: msend mreceive 

msend: msend.c
	gcc -o msend msend.c

mreceive: mreceive.c
	gcc -o mreceive mreceive.c

clean:
	rm -f msend mreceive

