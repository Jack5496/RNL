/*
        demo-udp-03: udp-send: a simple udp client
	send udp messages
	This sends a sequence of messages (the # of messages is defined in MSGS)
	The messages are sent to a port defined in SERVICE_PORT 

        usage:  udp-send

        Paul Krzyzanowski
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <time.h>


#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

#define BUFLEN 65536
#define MSGS 5	/* number of messages to send */


char server_adress[65536]; /* Platz für Server Adresse */
int server_port = 80;  /* Server Port */

void printSatus(int success, int pending){
		printf("\33[2K\r");
		int x;
		for(x=0;x<success;x++){
			printf(GREEN "•" RESET,stdout);
		}
		for(x=0;x<pending;x++){
			printf(YELLOW "•" RESET,stdout);
		}
		rewind(stdout);
}


int main(int argc, char **argv){

	struct sockaddr_in myaddr, remaddr;
	int fd, i, slen=sizeof(remaddr);
	char buf[BUFLEN];	/* message buffer */
	int recvlen;		/* # bytes in acknowledgement message */

	int needed_arguments = 1; //programm selber
	needed_arguments++; //Server Adress
    	needed_arguments++; //Server Port
    	needed_arguments++; //Mode
    	needed_arguments++; //From Here up the Message



	if(argc!=needed_arguments){
		printf("Not enough arguments!\n");
		printf("Usage: ServerAdress Port Mode Message!\n");
		exit(1);
	}

	
       int err_adr = inet_aton(argv[1],&remaddr.sin_addr);
        	
	if(err_adr==0) {
 		printf("Error: No valid Server IP\n");
     		exit(1);
 	}
        
        i=0;
        /* Laufe bis zum ende des Strings */
        while(argv[2][i] != '\0'){
             /* Überprüfe jedes Zeichen ob es eine Zahl ist in ASCII */
             if (argv[2][i] < 47 || argv[2][i] > 57){ //falls nicht
                printf("Error: No valid Port\n");
                exit(1);
            }
            i++;
        }
         
        /* Alles lief wohl gut */ 
        server_port = atoi(argv[2]);


	/* create a socket */

	if ((fd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		printf("socket created\n");

	/* bind it to all local addresses and pick any port number */

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}       

	/* now define remaddr, the address to whom we want to send messages */
	/* For convenience, the host address is expressed as a numeric IP address */
	/* that we will convert to a binary format via inet_aton */

	//memset((char *) &remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(server_port);

	/* now let's send the messages */

	int pending = 0;
	int success = 0;

	printf(GREEN "Test: " RESET "Sending %d messages to port %d\n",MSGS,server_port);

	for (i=1; i < MSGS+1; i++) {
		pending++;
		
		printSatus(success, pending);

		sprintf(buf, "This is packet %d", i);
		if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1) {
			perror("sendto");
			exit(1);
		}
		sleep(1);


		/* now receive an acknowledgement from the server */
		recvlen = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&remaddr, &slen);
              if (recvlen >= 0) {
                    	buf[recvlen] = 0;	/* expect a printable string - terminate it */
                        
			success++;
			pending--;
			printSatus(success, pending);   
			sleep(1);
			
			//printf("received message: \"%s\"\n", buf);
              }
	}
	printf("\nTest complete!\n");

	close(fd);
	return 0;
}
