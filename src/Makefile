server_obj = server.o
client_obj = client.o

CFLAGS = -g

CC = gcc

server: $(server_obj)
	$(CC) $(server_obj) -o server

server.o: server.c
	$(CC) -g -c server.c

client: $(client_obj)
	$(CC) -g -o client $(client_obj)

client.o: client.c
	$(CC) -c client.c

.PHONY: clean
clean:
	rm -rf $(server_obj) $(client_obj) server client
