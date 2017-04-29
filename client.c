/*
Auslastungsmess-Tool: Client
Dieser Client dient zum Testen eines Servers. Dabei kann in 4 Modi getestet werden.
Dieses Programm basiert auf der Grundlage von 
https://www.cs.rutgers.edu/~pxk/417/notes/sockets/demo-udp-04.html und wurde 
daher modifiziert. In dem Nachfolgenden Code können 4 Parameter verändert werden, 
diese befinden sich in dem Abschnitt: DEFAULTS

Benutzung: ./client <SERVER_IP> <PORT> <MODE> <[Modusparameter]>

Die Modi:
- 1: 	Dieser Modus misst die RoundTripTime (RTT), die Zeit welche die Pakete im Durchschnitt 
	benötigen um vom Client zum Server und zurück benötigen.
	Dabei wird die Zeit zunächst gestartet und das erste von <Param 1> Paketen versendet.
	Die Zeit wird gestoppt, sobald ein Packet zurück kommt. Dieses Paket wird allerdings
	nicht auf korrekten Inhalt geprüft und ist daher anfällig gegenüber zufälligen Paketen.
	Die Zeit Differenz wird zwischengespeichert. 
	Dieser Vorgang wird <Param 1> mal wiederholt, wobei jede Zeit Differenz auf den alten
	Wert aufaddiert wird.
	Es gibt einen Timeout für nicht ankommende Pakete, diese werden jedoch nicht in die
	RTT einberechnet.
	Nachdem alle Pakete angekommen oder verloren gegangen sind wird die RTT berechnet,
	indem die summierte Zeit durch die Anzahl der angekommenen Pakete dividiert wird.

	Benutzung: ./client <SERVER_IP> <PORT> 1 <Paketanzahl>

- 2:	Dieser Modus berechnet die maximale Bandbreite. Dabei sendet der Client ein Paket zum
	Server welcher erkennt, dass es sich um den Modus 2 handelt. Daraufhin entnimmt der 
	Server die Anzahl der angefragt Pakete <Param 2>. Der Server sendet nun <Param 2> viele
	Pakete dem Client zu, so schnell wie möglich. Jedes Paket besitzt dabei die Größe <MAXMTU> (200 Byte).
	Diese Größe richtet sich nach der Payload size von:
	http://www.cisco.com/c/en/us/support/docs/voice/voice-quality/7934-bwidth-consume.html#anc5
	Nachdem das erste Paket beim Client angekommen ist, wird die Zeitmessung gestartet.
	Da hierbei Packageloss entstehen kann, wie in den Versuchen bemerkt, zählt der Client nur <Param 1>
	viele Pakete. Für jedes Paket wird die angekommene Größe in Byte zwischen gespeichert und aufsummiert.
	Wenn all diese Pakete angekommen sind, wird die Zeit gestoppt. 
	Nun wird der Durchsatz in MB/s berechnet, indem die angekommen Byte aufsummiert werden und durch
	die Zeitdifferenz geteilt werden. Anschließend wird noch in MB/s umgerechnet.
	Das große Problem bei diesem Test stellt sich, wie in den Versuchen bemerkt, wenn zu wenig Pakete
	angekommen sind. Durch einen Timeout werden die verlorenen Pakete angezeigt, sodass der Test enden kann.
	Die Frage ist dabei, ob der Client nicht genug Pakete annehmen konnte, oder die Pakete wirklich abhanden
	gekommen sind. Dieser Test bietet sich daher an, wenn die angeforderte Anzahl der Pakete größer ist, als
	die zu zählenden Pakete. Sollten weniger Pakete ankommen, als die zu zählende Anzahl, bietet sich der Test
	nicht mehr an.

	Benutzung: ./client <SERVER_IP> <PORT> 2 <Zu Zählende Pakete> <Angeforderte Pakete>

- 3:	Dieser Modus berechnet den Packageloss. Dabei sendet wie in Modus 2 der Server eine angeforderte Zahl Paketen <Param 1> an den
	Client zurück. Jedes nicht ankommende Paket wird durch einen Timeout gezählt. Nachdem alle Pakete angekommen oder
	verloren sind, wird der Verlust prozentual berechnet.

	Benutzung: ./client <SERVER_IP> <PORT> 3 <Angeforderte Pakete>

- 4:	Dieser Modus ist direkt an die Benutzung von VoIP gerichtet. Bei VoIP werden in der Regel 90kbps bis 156kbps benötigt:
	https://getvoip.com/blog/2012/10/30/what-is-the-minimum-required-speed-for-voip/
	Durch Versuche eine maximale Bandbreite zu errechnen in Modus 2 kam es dazu, dass nach einer kurzen Zeit keine Pakete mehr
	angekommen sind und alle verloren gingen.
	Daher kam die Idee der Restriktion der Bandbreite auf 200 kbp/s —> 0,025 MB/s. Dazu wird mit der Paketgröße
	 <MAXMTU> (200 Byte) berechnet wie viele pro Sekunde versendet werden müssen. Diese werden in dem berechneten
	Zeitintervall nacheinander versendet. Abschließend wird wie in Modus 2 die Bandbreite berechnet.
*/



#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h> // for close
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#include <math.h>

/* DEFAULTS: */
#define TIMEOUT 3
#define DEFAULT_RTT_COUNTING 100 /* Default counting package value */
#define DEFAULT_BANDWIDTH_COUNTING 30 /* Default counting package value */
#define DEFAULT_LOSS_COUNTING 100 /* Default counting package value */
// Potter 131.173.33.201


/* Some colors */
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"
/* End colors */

/* Some Variables */
#define BUFLEN 65536 	/* Buffer length for sending and recieving */
#define MAXMTU 200 		/* Maximum Package size */
#define TO_SEC 1000000	/* number of messages to send */
#define TO_MS 1000		/* number of messages to send */
char server_adress[65536]; 	/* Server Adress */
int server_port;  	/* Server Port */

struct sockaddr_in myaddr, remaddr; /* Server und Client SockAddr*/
int fd, i;
socklen_t slen=sizeof(remaddr);	 
char buf[BUFLEN];	/* message buffer */
int recvlen;		/* # bytes in acknowledgement message */
int mode;		/* Modus Variable */
int rtt_mode = 1;	
int bandwith_mode = 2;
int packageloss_mode = 3;
int voip_mode = 4;

/* Calculate Variables */
double roundtt = 0.0;
double loss = 0.0;
double voipgoodput = 0.0;
double goodput = 0.0;

/* Time Variables */
struct timespec timeStart, timeEnd;
double totaltime = 0;
double deltatime = 0;


/*
Gibt einen Status in der selben Zeile zurück nach dem Schema:
"Recieved/Total | Lost" mit den drei Parametern in dieser Reihenfolge
*/
void printStatus(int success, int total_messages, int lost){
		printf("\33[2K\r");	/* Springe zum Anfang der Zeile zurück */
		printf("Recieved: " GREEN "%d" RESET"/%d | Lost: "RED"%d"RESET,success,total_messages,lost,stdout);
		rewind(stdout); /* Springe zum Anfang der Zeile zurück */
}

/*
Einfache Methode, welche eine Farbige Fehlernachricht auf der Konsole ausgibt
Und danach das Programm terminiert, sowie zuvor noch offene Sockets schließt.
@param *argv - Die Fehlernachricht
*/
int myerror(char *argv){
	printf(RED "%s" RESET "\n",argv);
	if(fd>0){
		close(fd);
	}
	exit(1);
}

/*
Überprüft einen gegebenen String, ob dieser eine Zahl ist.
Sofern es sich um eine handelt, wird diese zurück gegeben.
Wenn nicht, terminiert das Programm.
@param *argv - String welcher eine Zahl sein soll
*/
int stringToInt(char *argv){
	int i=0;
        /* Laufe bis zum ende des Strings */
        while(argv[i] != '\0'){
             /* Überprüfe jedes Zeichen ob es eine Zahl ist in ASCII */
             if (argv[i] < 47 || argv[i] > 57){ //falls nicht
                myerror("Error: No valid Number\n");
                
            }
            i++;
        }

	return atoi(argv);	/* Rückgabe */
}

/*
Gibt das RTT Ergebnis auf der Konsoleaus, 
welches zuvor in der Methode rtt(amount) berechnet wurde
*/
int printRTTResult(){
	printf("Avg. RTT: "GREEN"%g"RESET"ms\n", roundtt);
	return 0;
}

/*
Berechnet die RTT nach dem im Anfang beschriebenen Schema.
@param int amount - Anzahl der zu seidenen Pakete
*/
int rtt(int amount){
	printf(GREEN "RTT Test: " RESET "Sending %d messages\n",amount);

	int lost = 0;
	
	/* Sende amount viele Pakete */
	for (i=1; i < amount +1; i++) {
		/*Fülle Buffer*/
		sprintf(buf, "[%d] This is packet %d", rtt_mode, i);

		/* Starte Zeit mit Fehlerbehandlung */
		if(clock_gettime(CLOCK_MONOTONIC_RAW,&timeStart)){
			myerror("Time is broken!\n");
		}
		
		/* Sende Buffer an den Server mit Fehlerbehandlung */
		if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1) {
			perror("sendto");
			myerror("SendTo Error!");
		}

		/* Warte auf ein ankommendes Paket */
		recvlen = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&remaddr, &slen);
              if (recvlen >= 0) {
			/* Wenn was angekommen ist, stopp die Zeit */
			if(clock_gettime(CLOCK_MONOTONIC_RAW,&timeEnd)){
				myerror("Time is broken!");
			}
			
			/* Berechne die Zeit */
			deltatime = (timeEnd.tv_sec - timeStart.tv_sec) * TO_SEC;
			deltatime += (timeEnd.tv_nsec - timeStart.tv_nsec) / TO_MS;
			totaltime += deltatime;
                        
			printStatus(i, amount,lost);   
			
              }
		else{	
			/* Ist das Paket verloren gegangen, notiere dies */
			lost++;
			printStatus(i, amount,lost);
		}
	}

	/* Berechne RTT nach Schema oben */
	roundtt = totaltime/(amount-lost)/TO_MS;
	
	printStatus(amount-lost, amount,lost);
	printf("\nTest complete!\n");

	return 0;
}

/*
Gibt die berechnete Bandbreite auf der Konsole aus 
*/
int printBandwithResult(){
	printf("Bandwidth: "GREEN"%g"RESET"MB/s\n", goodput);
	return 0;
}

/*
Berechnet die Bandbreite nach dem obigen Schema
@param int counting - Anzahl zählende Pakete
@param int demand - Anzahl anfragende Pakete
*/
int bandwith(int counting, int demand){
	printf(GREEN "Bandwidth Test: " RESET "Demanding %d packages, counting %d\n",demand,counting);

	/* Imitieren der Variablen */
	int lost = 0;
	goodput = 0.0;
	totaltime = 0.0;

	/* Erstelle Buffer mit 1 gefüllt */
	char data[MAXMTU];
	int pos;
	for(pos=0;pos<MAXMTU;pos++){
		data[pos]='1';
	}
	data[MAXMTU-1]='\0'; /* Beende den String */
	/* Buffer könnte dazu genutzt werden Bitfehler zu erkennen */

	/* Befülle zu sendenen Buffer */
	sprintf(buf, "[%d][%d]%s", bandwith_mode,demand,data);

	/* Sende Paket los, allerdings mit maximaler Länge MAXMTU */
	if (sendto(fd, buf, MAXMTU, 0, (struct sockaddr *)&remaddr, slen)==-1) {
			perror("sendto");
			myerror("SendTo Error!");
		}

	/* Zähle counting viele Pakete */
	for (i=1; i < counting+1; i++) {


		/* Warte auf Paket */
		recvlen = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&remaddr, &slen);
              if (recvlen >= 0) {
			/* Sichere Paketgröße */
			goodput+=recvlen;
		
			//Ausgabetest
                    	//buf[recvlen] = '\0';	/* Beende Buffer mit terminierungssymbol */

                     printStatus(i,counting,lost);
              }
		else{	
			/* Zähle verlorene Pakete */
			lost++;
			printStatus(i-lost,counting,lost);
		}

		if(i==1){
			/* Starte Zeit bei erhalt des ersten Paket */
			if(clock_gettime(CLOCK_MONOTONIC_RAW,&timeStart)){
				myerror("Time is broken!\n");
			}
		}
		if(i==counting){
			/* Beende Zeit bei erhalt des letzten Paket */
			if(clock_gettime(CLOCK_MONOTONIC_RAW,&timeEnd)){
				myerror("Time is broken!");
			}
		}
	}

	/* Berechne benötigte Zeit */
	deltatime = (timeEnd.tv_sec - timeStart.tv_sec) * TO_SEC;
	deltatime += (timeEnd.tv_nsec - timeStart.tv_nsec) / TO_MS;
	totaltime += deltatime;
   	totaltime/=TO_SEC;

	/* Wandel Goodput um */
	goodput/=1000000; //in MB

	/* Berechne Bandbreite */
	goodput/=totaltime;
	printStatus(counting-lost, counting,lost);
	printf("\nTest complete!\n");

	return 0;
}

/*
Gebe Packageloss auf der Konsole aus
*/
int printPackagelossResult(){
	printf("Packageloss: "GREEN"%g"RESET"%%\n", loss);
	return 0;
}

/*
Berechne Packageloss
*/
int packageloss(int amount){

	/* Initiierung der Variablen */
	int lost = 0;
	loss = 0.0;

	printf(GREEN "Packageloss Test: " RESET "Demanding %d messages\n",amount);

	/* Befühle Payload */
	char data[MAXMTU];
	int pos;
	for(pos=0;pos<MAXMTU;pos++){
		data[pos]='1';
	}
	data[MAXMTU-1]='\0';

	/* Befühle Buffer */
	sprintf(buf, "[%d][%d] - %s", packageloss_mode,amount, data);

	/* Sende Paket */
	if (sendto(fd, buf, MAXMTU, 0, (struct sockaddr *)&remaddr, slen)==-1) {
			perror("sendto");
			myerror("SendTo Error!");
	}
	
	/* Warte auf alle Pakete */
	for (i=1; i < amount+1; i++) {
		/* now receive an acknowledgement from the server */
		recvlen = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&remaddr, &slen);
              if (recvlen >= 0) {
                    	printStatus(i-lost, amount,lost);
              }
		else{
			/* Registriere Verlorenes Paket */
			lost++;
			printStatus(i-lost, amount,lost);
		}
	}
   	
	printStatus(amount-lost, amount,lost);
	printf("\nTest complete!\n");
	loss = lost/amount;

	return 0;
}

/*
Gebe VoIP Ergebnis auf der Konsole aus
*/
int printVoipResult(){
	printf("VoIP Bandwidth: "GREEN"%g"RESET"MB/s\n", voipgoodput);
	if(voipgoodput>0.019){
		printf("VoIP: "GREEN"GOOD"RESET"\n");
	}
	else if(voipgoodput>0.011){
		printf("VoIP: "YELLOW"OK"RESET"\n");
	}
	else{
		printf("VoIP: "RED"IMPOSSIBLE"RESET"\n");
	}

	return 0;
}

/* stable VoIP at 156 kbps —> 0,0195MB/s */
/*
Berechne VoIP nach obigen Schema
@param int counting - Anzahl zählende Pakete
@param int demand - Anzahl anfragende Pakete
*/
int voip(int counting, int demand){
	printf(GREEN "VoIP Test: " RESET "Demanding %d packages, counting %d\n",demand,counting);

	/* Initiere Variablen */
	int lost = 0;
	voipgoodput = 0.0;
	totaltime = 0.0;

	/* Verfülle Payload */
	char data[200];
	int pos;
	for(pos=0;pos<MAXMTU;pos++){
		data[pos]='1';
	}
	data[MAXMTU-1]='\0';
	
	/* Befühle Buffer */
	sprintf(buf, "[%d][%d]%s", voip_mode,demand,data);

	/* Sende Paket */
	if (sendto(fd, buf, MAXMTU, 0, (struct sockaddr *)&remaddr, slen)==-1) {
			perror("sendto");
			myerror("SendTo Error!");
	}

	/* Warte auf alle Pakete */
	for (i=1; i < counting+1; i++) {


		/* now receive an acknowledgement from the server */
		recvlen = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&remaddr, &slen);
              if (recvlen >= 0) {
			/* Sichere Paketgröße */
			voipgoodput+=recvlen;
                     printStatus(i,counting,lost);
              }
		else{
			/* Bemerke Paketverlust */
			lost++;
			printStatus(i-lost,counting,lost);
		}

		/* Starte Zeit bei erhalt erstes Pakets */
		if(i==1){
			if(clock_gettime(CLOCK_MONOTONIC_RAW,&timeStart)){
				myerror("Time is broken!\n");
			}
		}

		/* Stoppe Zeit bei erhalt letztes Pakets */
		if(i==counting){
			if(clock_gettime(CLOCK_MONOTONIC_RAW,&timeEnd)){
				myerror("Time is broken!");
			}
		}
	}

	/* Berechne Zeit */
	deltatime = (timeEnd.tv_sec - timeStart.tv_sec) * TO_SEC;
	deltatime += (timeEnd.tv_nsec - timeStart.tv_nsec) / TO_MS;
	totaltime += deltatime;
   	totaltime/=TO_SEC;

	/* Berechne VoIP Goodput */
	voipgoodput/=1000000; //in MB

	voipgoodput/=totaltime;
	printStatus(counting-lost, counting,lost);
	printf("\nTest complete!\n");

	return 0;
}


/* 
Main Programm welches zur Messung von Netzwerken genutzt werden kann
 */
int main(int argc, char **argv){
	
	int needed_arguments = 1; //programm selber
	needed_arguments++; //Server Adress
    	needed_arguments++; //Server Port
    	needed_arguments++; //Mode

	int auto_test = needed_arguments-1;
	
	int arguments_for_rtt = needed_arguments;
	arguments_for_rtt ++; //Amount
	
	int arguments_for_brandwith = needed_arguments;
	arguments_for_brandwith ++; //Counting Package
	arguments_for_brandwith ++; //Server Sending

	int arguments_for_packageloss = needed_arguments;
	arguments_for_packageloss ++; //Amount

	/* Genug Argumente gegeben? */
	if(argc<auto_test){
		printf("Not enough arguments!\n");
		exit(1);
	}

	/* Setzen der Adresse */
       int err_adr = inet_aton(argv[1],&remaddr.sin_addr);
        	
	if(err_adr==0) {
 		myerror("Error: No valid Server IP");
 	}
         
       /* Port auslesen */ 
       server_port = stringToInt(argv[2]);

	/* create a socket */
	if ((fd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		printf("socket created\n");

	/* bind it to all local addresses and pick any port number */
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0);


	/* Setze Timeout */
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    		perror("Error");
	}

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		myerror("Binding Failed!");
	}       

	/* now define remaddr, the address to whom we want to send messages */
	/* For convenience, the host address is expressed as a numeric IP address */
	/* that we will convert to a binary format via inet_aton */
	//memset((char *) &remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(server_port);



	/* Sollen alle Tests ausgeführt werden mit Standart Werten */
	if(argc==auto_test){
		rtt(DEFAULT_RTT_COUNTING);
		printRTTResult();

		voip(DEFAULT_BANDWIDTH_COUNTING, DEFAULT_BANDWIDTH_COUNTING);
		printVoipResult();
		
		packageloss(DEFAULT_LOSS_COUNTING);
		printPackagelossResult();
		
		bandwith(DEFAULT_BANDWIDTH_COUNTING,2* DEFAULT_BANDWIDTH_COUNTING);
		printBandwithResult();
	}	
	else{
		
		/* Mode auslesen */
       	mode = stringToInt(argv[3]);
	
		/* Überprüfen ob valider Mode */
		if(mode<1 || mode > 4){
			myerror("Mode has to be [1..4] !");
		}

		/* Ausführen des TestMode mit Überprüfung auf Argumente */

		if(mode==rtt_mode){
			if(argc!=arguments_for_rtt){
				myerror("Not enough arguments for RRT!");
			}
			rtt(stringToInt(argv[4]));
			printRTTResult();
		}
		if(mode==bandwith_mode){
			if(argc!=arguments_for_brandwith){
				myerror("Not enough arguments for brandwith!");
			}
			bandwith(stringToInt(argv[4]),stringToInt(argv[5]));
			printBandwithResult();
		}
		if(mode==packageloss_mode){
			if(argc!=arguments_for_packageloss){
				myerror("Not enough arguments for package loss!");
			}
			packageloss(stringToInt(argv[4]));
			printPackagelossResult();
		}
		if(mode==voip_mode){
			if(argc!=arguments_for_brandwith){
				myerror("Not enough arguments for voip!");
			}
			voip(stringToInt(argv[4]),stringToInt(argv[5]));
			printVoipResult();
		}

	}

	/* Schließen des Socket */
	close(fd);
	return 0;


	
}
