all: msend mreceive ttcp

msend: msend.c
	gcc -o msend msend.c

mreceive: mreceive.c
	gcc -o mreceive mreceive.c

ttcp: ttcp.c
	gcc -o ttcp ttcp.c

clean:
	rm -f msend mreceive

