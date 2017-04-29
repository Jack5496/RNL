binaries = server client

all: server client

server: server.c
	gcc -o server server.c -Wall -Werror
	
client: client.c
	gcc -o client client.c -Wall -Werror

clean: 
	rm -f $(binaries) *.o
