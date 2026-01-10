all: server

server: open62541.o server.o
	gcc -o server server.o open62541.o -lmbedtls -lmbedx509 -lmbedcrypto

server.o: server.c
	gcc -std=c99 -I../../ext/opcua_client -c server.c

open62541.o:
	gcc -std=c99 -c ../../ext/opcua_client/open62541.c

clean:
	-rm *.o server
