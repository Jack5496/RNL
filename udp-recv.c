/*
        demo-udp-03: udp-recv: a simple udp server
	receive udp messages
*/

#include <signal.h> /* Damit ich Signale abfangen kann */
#include <unistd.h> /* für strg c anfangen */
#include <errno.h>   /* errno             */
#include <locale.h>  /* locale support    */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSIZE 2048

int SERVICE_PORT = 5050; /* default port */
int keep_alive = 1; /* boolean solange zugehört wird */
char server_adress[] = "127.0.0.1";  /* Default server adresse */

/**
* Letzer aufruf um alles wichtige zu schließen
*/
void last_wish(int i){
	printf("\nManuelles Beenden\n");
	keep_alive=0; /* Wir möchten unsere While Loop beenden */
}

int
main(int argc, char **argv)
{
	//Handlet aktivierung für STRG+C
	//http://stackoverflow.com/questions/1641182/how-can-i-catch-a-ctrl-c-event-c
	struct sigaction sigIntHandler;
	int i;

	sigIntHandler.sa_handler = last_wish;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0; //setze sa flags 0

	sigaction(SIGINT, &sigIntHandler, NULL);
	// Ende für STRG+C




	int needed_arguments=1; //Programm selber
	needed_arguments++; //Server Port

	/* Prüfe ob genug Argumente vorhanden */
	if(argc==needed_arguments){       
		int err_port = 0;
		i=0;
		while(argv[1][i] != '\0'){
			/* Prüfe ob der Port nur aus Zahlen besteht */
			if (argv[1][i] < 47 || argv[1][i] > 57){
				err_port = 1;
				break;
			}
			i++;
		}
		if(err_port==1){
			printf("Error: No valid Port\n");
			return 1;
		}        
		SERVICE_PORT = atoi(argv[1]);
	}
	else{
		printf("No Port defined\n");
		return 1;
	}

		//Einfacher UDP Server http://www.programminglogic.com/sockets-programming-in-c-using-udp-datagrams/



	struct sockaddr_in myaddr,remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen;			/* # bytes received */
	int fd;				/* our socket */
	int msgcnt = 0;			/* count # of messages we received */
	unsigned char buf[BUFSIZE];	/* receive buffer */


	/* create a UDP socket */

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}

	/* bind the socket to any valid IP address and a specific port */

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(SERVICE_PORT);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}

	/* now loop, receiving data and printing what we received */
	while(keep_alive) {
		printf("Waiting on port %d\n", SERVICE_PORT);
		recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
		if (recvlen > 0) {
			buf[recvlen] = '\0';
			printf("received message: \"%s\" (%d bytes)\n", buf, recvlen);
		}
		else
			printf("uh oh - something went wrong!\n");
		sprintf(buf, "ack %d", msgcnt++);
		printf("sending response \"%s\"\n", buf);
		if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
			perror("sendto");
	}
	/* never exits */



	}
