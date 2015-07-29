all: msend mreceive 

msend: msend.source/msend.c
	gcc -o msend msend.source/msend.c

mreceive: mreceive.source/mreceive.c
	gcc -o mreceive mreceive.source/mreceive.c

clean:
	rm -f msend mreceive

