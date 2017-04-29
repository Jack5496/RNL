binaries = pa3_server pa3_client

all: pa3_server pa3_client

pa3_server: pa3_server.c
	gcc -o pa3_server pa3_server.c -Wall -Werror -lgpgme
	
pa3_client: pa3_client.c
	gcc -o pa3_client pa3_client.c -Wall -Werror -lgpgme

clean: 
	rm -f $(binaries) *.o
