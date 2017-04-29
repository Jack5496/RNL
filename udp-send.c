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
#include <inttypes.h>
#include <math.h>


#define TIMEOUT 30
#define DEFAULT_RTT_COUNTING 100 /* Default counting package value */
#define DEFAULT_BANDWIDTH_COUNTING 50 /* Default counting package value */
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

/* Some variables */
#define BUFLEN 65536 /* Buffer length for sending and recieving */
#define MAXMTU 1500 /* Maximum Package size */
#define MSGS 100	/* number of messages to send */
#define TO_SEC 1000000	/* number of messages to send */
#define TO_MS 1000	/* number of messages to send */

char server_adress[65536]; /* Server Adress */ // Potter 131.173.33.201
int server_port = 80;  /* Server Port */

void printStatus(int success, int total_messages, int lost){
		printf("\33[2K\r");
		printf("Recieved: " GREEN "%d" RESET"/%d | Lost: "RED"%d"RESET,success,total_messages,lost,stdout);
		rewind(stdout);
}

struct sockaddr_in myaddr, remaddr;
int fd, i, slen=sizeof(remaddr);
char buf[BUFLEN];	/* message buffer */
int recvlen;		/* # bytes in acknowledgement message */
int mode;
int rtt_mode = 1;
int bandwith_mode = 2;
int packageloss_mode = 3;

double roundtt = 0.0;
double goodput = 0.0;
double loss = 0.0;

struct timespec timeStart, timeEnd;
double totaltime = 0;
double deltatime = 0;

int myerror(char *argv){
	printf(RED "%s" RESET "\n",argv);
	exit(1);
}

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

	return atoi(argv);
}

int printRTTResult(){
	printf("Avg. RTT: "GREEN"%g"RESET"ms\n", roundtt);
	return 0;
}

int rtt(int amount){

/* now let's send the messages */

	printf(GREEN "RTT Test: " RESET "Sending %d messages\n",amount);

	int lost = 0;

	for (i=1; i < amount +1; i++) {

		sprintf(buf, "[%d] This is packet %d", rtt_mode, i);

		if(clock_gettime(CLOCK_MONOTONIC_RAW,&timeStart)){
			myerror("Time is broken!\n");
		}

		if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1) {
			perror("sendto");
			myerror("SendTo Error!");
		}


		/* now receive an acknowledgement from the server */
		recvlen = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&remaddr, &slen);
              if (recvlen >= 0) {
			if(clock_gettime(CLOCK_MONOTONIC_RAW,&timeEnd)){
				myerror("Time is broken!");
			}

			deltatime = (timeEnd.tv_sec - timeStart.tv_sec) * TO_SEC;
			deltatime += (timeEnd.tv_nsec - timeStart.tv_nsec) / TO_MS;
			totaltime += deltatime;

                    	buf[recvlen] = 0;	/* expect a printable string - terminate it */
                        
			printStatus(i, amount,lost);   
			
              }
		else{
			lost++;
			printStatus(i, amount,lost);
		}
	}

	roundtt = totaltime/amount;
	printStatus(amount-lost, amount,lost);
	printf("\nTest complete!\n");

	return 0;
}





int printBandwithResult(){
	printf("Bandwidth: "GREEN"%g"RESET"MB/s\n", goodput);
	return 0;
}

int bandwith(int counting, int demand){

/* now let's send the messages */

	printf(GREEN "Bandwidth Test: " RESET "Demanding %d packages, counting %d\n",demand,counting);

	int lost = 0;
	goodput = 0.0;
	totaltime = 0.0;

	char data[MAXMTU];
	int pos;
	for(pos=0;pos<MAXMTU;pos++){
		data[pos]='1';
	}
	data[MAXMTU-1]='\0';

	sprintf(buf, "[%d][%d]%s", bandwith_mode,demand,data);

	if (sendto(fd, buf, MAXMTU, 0, (struct sockaddr *)&remaddr, slen)==-1) {
			perror("sendto");
			myerror("SendTo Error!");
		}

	for (i=1; i < counting+1; i++) {


		/* now receive an acknowledgement from the server */
		recvlen = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&remaddr, &slen);
              if (recvlen >= 0) {

			goodput+=recvlen;

                    	buf[recvlen] = 0;	/* expect a printable string - terminate it */
                     printStatus(i,counting,lost);
              }
		else{
			lost++;
			printStatus(i-lost,counting,lost);
		}

		if(i==1){
			if(clock_gettime(CLOCK_MONOTONIC_RAW,&timeStart)){
				myerror("Time is broken!\n");
			}
		}
		if(i==counting){
			if(clock_gettime(CLOCK_MONOTONIC_RAW,&timeEnd)){
				myerror("Time is broken!");
			}
		}
	}

	deltatime = (timeEnd.tv_sec - timeStart.tv_sec) * TO_SEC;
	deltatime += (timeEnd.tv_nsec - timeStart.tv_nsec) / TO_MS;
	totaltime += deltatime;
   	totaltime/=TO_SEC;
	goodput/=8000000; //in MB

	goodput/=totaltime;
	printStatus(counting-lost, counting,lost);
	printf("\nTest complete!\n");

	return 0;
}


int printPackagelossResult(){
	printf("Packageloss: "GREEN"%g"RESET"%%\n", loss);
	return 0;
}

int packageloss(int amount){

/* now let's send the messages */
	int lost = 0;
	loss = 0.0;

	printf(GREEN "Test: " RESET "Demanding %d messages\n",amount);

	char data[MAXMTU];
	int pos;
	for(pos=0;pos<MAXMTU;pos++){
		data[pos]='1';
	}
	data[MAXMTU-1]='\0';

	sprintf(buf, "[%d][%d] - %s", packageloss_mode,amount, data);

	if (sendto(fd, buf, MAXMTU, 0, (struct sockaddr *)&remaddr, slen)==-1) {
			perror("sendto");
			myerror("SendTo Error!");
	}

	for (i=1; i < amount+1; i++) {
		/* now receive an acknowledgement from the server */
		recvlen = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&remaddr, &slen);
              if (recvlen >= 0) {
                    	buf[recvlen] = 0;	/* expect a printable string - terminate it */
         

			printStatus(i-lost, amount,lost);
              }
		else{
			lost++;
			printStatus(i-lost, amount,lost);
		}
	}
   	
	printStatus(amount-lost, amount,lost);
	printf("\nTest complete!\n");
	loss = lost/amount;

	return 0;
}




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

	if(argc<auto_test){
		printf("Not enough arguments!\n");
		exit(1);
	}

	
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



	
	if(argc==auto_test){
		rtt(DEFAULT_RTT_COUNTING);
		bandwith(DEFAULT_BANDWIDTH_COUNTING,2* DEFAULT_BANDWIDTH_COUNTING);
		packageloss(DEFAULT_LOSS_COUNTING);

		printRTTResult();
		printBandwithResult();
		printPackagelossResult();
	}	
	else{
		
		/* Mode auslesen */ 
       mode = stringToInt(argv[3]);
	
	if(mode<1 || mode > 3){
		myerror("Mode has to be [1..3] !");
	}
	if(mode==1){
		if(argc!=arguments_for_rtt){
			myerror("Not enough arguments for RRT!");
		}
		rtt(stringToInt(argv[4]));
		printRTTResult();
	}
	if(mode==2){
		if(argc!=arguments_for_brandwith){
			myerror("Not enough arguments for brandwith!");
		}
		bandwith(stringToInt(argv[4]),stringToInt(argv[5]));
		printBandwithResult();
	}
	if(mode==3){
		if(argc!=arguments_for_packageloss){
			myerror("Not enough arguments for package loss!");
		}
		packageloss(stringToInt(argv[4]));
		printPackagelossResult();
	}


	}


	close(fd);



	
}
