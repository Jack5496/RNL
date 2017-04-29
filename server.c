/*
Der Server zum Messen eines Netzwerk. Dieses Programm ist das Gegenstück des Client und führt die dort beschriebenen Handlungen aus.
Benutzung: ./server <PORT>
*/

#include <signal.h> /* Damit ich Signale abfangen kann */
#include <unistd.h> /* für strg c anfangen */
#include <errno.h>   /* errno             */
#include <locale.h>  /* locale support    */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSIZE 65536

/* Colors */
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

int SERVICE_PORT = 5050; /* default port */
int keep_alive = 1; /* boolean solange zugehört wird */
char server_adress[] = "127.0.0.1";  /* Default server adresse */
char buf[BUFSIZE];	/* receive buffer */

struct sockaddr_in myaddr,remaddr;	/* remote address */
socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
int recvlen;			/* # bytes received */
int fd;				/* our socket */
int msgcnt = 0;			/* count # of messages we received */	
int mode;			/* Modus */

/* Printet einen Status aus */
void printStatus(int success, int total_messages){
		printf("\33[2K\r");
		printf("Sending: " GREEN "%d" RESET"/%d",success,total_messages);
		rewind(stdout);
}

/**
* Letzer aufruf um alles wichtige zu schließen
*/
void last_wish(int i){
	printf("\nManuelles Beenden\n");
	keep_alive=0; /* Wir möchten unsere While Loop beenden */
}

/* 
Kleine farbige Fehlermeldung die das Programm anschließend beendet
 */
int myerror(char *argv){
	printf(RED "%s" RESET "\n",argv);
	exit(1);
}

/* 
Konvertiert String in ein Int um
 */
int stringToInt(char argv[]){
	int i=0;
        /* Laufe bis zum ende des Strings */
        while(argv[i] != '\0'){
             /* Überprüfe jedes Zeichen ob es eine Zahl ist in ASCII */
             if (argv[i] < 47 || argv[i] > 57){ //falls nicht
                myerror("Error: No valid Number\n");
                
            }
            i++;
        }

	return atoi(argv);
}

/* 
Extrahiert einen Parameter (Int) aus dem Buffer und gibt diesen Zurück
 */
int getParam(char *buffer,int pos){
	/* Positions Variablen */
	int from = -1;
	int till = -1;

	int i=0;
        /* Laufe bis zum ende des Strings */
        while(buffer[i] != '\0'){
             /* Überprüfe jedes Zeichen ob es eine Zahl ist in ASCII */
		if(pos<0){
			break;
		}
		/* [ gefunden? */
             	if (buffer[i] == 91){
			from = i;	
            	}
		/* ] gefunden? */
	    	if (buffer[i] == 93){
			till = i;
			pos--; /* skip Param */
            	}
            	i++;
        }

	/* Berechne tanzt große */
	int size = till-from-1;

	/* Erstelle Buffer und extrahiere */
	char subbuff[size+1];
	memcpy( subbuff, &buffer[from+1], size );
	subbuff[size] = '\0';

	/* Gebe Parameter zurück */
	int mode = stringToInt(subbuff);

	return mode;

}

/* 
Spiegele einfach die im Buffer angekommene Nachricht an den Client
 */
int rttResponse(){
	sprintf(buf, "ack %d", msgcnt++);
	//printf("sending response \"%s\"\n", buf);
	if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, addrlen) < 0){
		perror("sendto");
	}
	return 0;
}

/* Sende die angekommene Nachricht @param amount mal zurück */
int bandwithResponse(int amount){

	/* Da bei vielen Nachrichten zu wenig am Client ankamen,
	habe ich eine kleine Verzögerung einbaut, da ich denke dass dies
	das Problem mindert */
	int milliseconds = 1000;
	struct timespec ts;
    	ts.tv_sec = milliseconds / 1000;
    	ts.tv_nsec = (milliseconds % 1000) * 1000000;

	/* Make kurze Pause für den Client */
	nanosleep(&ts, NULL);

	/* Sende amount mal das Paket an den Client */
	int i;
	for(i=1;i<amount+1;i++){
		printStatus(i,amount);
		if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, addrlen) < 0){
			perror("sendto");
		}
	}
	printf("\n");
	
	return 0;
}

/* Sende das Paket des Client @amount mal zurück
Warte dabei zwischendurch, falls der Client zuviele Pakete
bekommt eine kurze Zeit */
int packagelossResponse(int amount){
	int i;
	
	/* Mache kurze Pause von 10mili. Sekunden */
	int milliseconds = 10;
	struct timespec ts;
    	ts.tv_sec = milliseconds / 1000;
    	ts.tv_nsec = (milliseconds % 1000) * 1000000;
    	

	/* Sende amount mal ein Paket los */
	for(i=1;i<amount+1;i++){
		printStatus(i,amount);
		if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, addrlen) < 0){
			perror("sendto");
		}
		nanosleep(&ts, NULL);
	}
	printf("\n");
	
	return 0;
}

/* very stable VoIP at 200 kbps —> 0,025MB/s  */
/* 0,0195MB/s /packagesize = packages/s */
/* Sendet Pakete an den Client mit einer Rate von ca. 0.025MB/s um
Eine VoIP Datenrate zu simulieren */
int voipResponse(int amount){
	double buflen = strlen(buf);	/* Hole Bufferlänge in Byte */
	double buflen_in_mb = buflen/1000000;	/* Rechne diese in MB um */
	double packages_per_second = 0.025/buflen_in_mb;	/* Berechne Paketanzahl pro Sekunde */
	double ms_to_wait = (1.0/packages_per_second)*1000.0; /* Berechne Delay zwischen Paketen */
	
	/* Kleine Debug ausgaben */
	//printf("Package/s: %g\n", packages_per_second);
	//printf("Waiting: %g\n", ms_to_wait);
	
	int i;

	/* Stelle Delay auf */
	int milliseconds = ms_to_wait;
	struct timespec ts;
    	ts.tv_sec = milliseconds / 1000;
    	ts.tv_nsec = (milliseconds % 1000) * 1000000;
    	
	/* Sende amount Pakete mit Delay */
	for(i=1;i<amount+1;i++){
		printStatus(i,amount);
		if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, addrlen) < 0){
			perror("sendto");
		}
		nanosleep(&ts, NULL);	/* Schlafe für Delay */
	}
	printf("\n");
	
	return 0;
}


/* Main Programm in welcher der Server auf ankommende Pakete wartet und diese gerecht nach dem
Angeforderten Modi verarbeitet */
int main(int argc, char **argv)
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

	//Grundlage von https://www.cs.rutgers.edu/~pxk/417/notes/sockets/demo-udp-04.html

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

			mode = getParam(buf,0); /* Extrahiere Modus */
			int amount = getParam(buf,1); /* Extrahiere Anzahl */

			printf("Received Message: Mode:%d | Amount: %d\n", mode, amount);

			/* Entscheide Nach Modus und Antworte */
			if(mode==1){
				rttResponse();
			}
			if(mode==2){
				bandwithResponse(amount);
			}
			if(mode==3){
				packagelossResponse(amount);
			}
			if(mode==4){
				voipResponse(amount);
			}
		}
		else{
			printf("uh oh - something went wrong!\n");
		}

	}

	/* Schließen des Socket */
	close(fd);
	return 0;

	}
