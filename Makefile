all: server client

server: server.c
	gcc server.c -o fserver

client: client.c
	gcc client.c -o fget
