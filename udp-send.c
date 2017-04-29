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


#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

#define BUFLEN 65536
#define MSGS 100	/* number of messages to send */
#define TO_SEC 1000000	/* number of messages to send */
#define TO_MS 1000	/* number of messages to send */


char server_adress[65536]; /* Platz für Server Adresse */
// Potter 131.173.33.201
int server_port = 80;  /* Server Port */

void printSatus(int success, int total_messages,double rtt){
		printf("\33[2K\r");
		printf("Recieved: " GREEN "%d" RESET "/%d | RTT in s: %g",success,total_messages,rtt,stdout);
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


int rtt(){

/* now let's send the messages */

	printf(GREEN "Test: " RESET "Sending %d messages to port %d\n",MSGS,server_port);

	for (i=1; i < MSGS+1; i++) {

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
				exit(1);
			}

			deltatime = (timeEnd.tv_sec - timeStart.tv_sec) * TO_SEC;
			deltatime += (timeEnd.tv_nsec - timeStart.tv_nsec) / TO_MS;
			totaltime += deltatime;

                    	buf[recvlen] = 0;	/* expect a printable string - terminate it */
                        
			printSatus(i, MSGS,deltatime/TO_SEC);   
			
              }
		else{
			myerror("RecFrom Fail!");
		}
	}

	double rtt = totaltime/MSGS/TO_SEC;
	printSatus(MSGS, MSGS,rtt);   
	printf("\nTest complete!\n");

	close(fd);
	return 0;
}








int bandwith(int amount, char *message){

/* now let's send the messages */

	printf(GREEN "Test: " RESET "Sending %d messages to port %d\n",MSGS,server_port);

	for (i=1; i < MSGS+1; i++) {

		sprintf(buf, "This is packet %d", i);

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
				exit(1);
			}

			deltatime = (timeEnd.tv_sec - timeStart.tv_sec) * TO_SEC;
			deltatime += (timeEnd.tv_nsec - timeStart.tv_nsec) / TO_MS;
			totaltime += deltatime;

                    	buf[recvlen] = 0;	/* expect a printable string - terminate it */
                        
			printSatus(i, MSGS,deltatime/TO_SEC);   
			
              }
		else{
			myerror("RecFrom Fail!");
		}
	}

	double rtt = totaltime/MSGS/TO_SEC;
	printSatus(MSGS, MSGS,rtt);   
	printf("\nTest complete!\n");

	close(fd);
	return 0;
}






int main(int argc, char **argv){
	
	int needed_arguments = 1; //programm selber
	needed_arguments++; //Server Adress
    	needed_arguments++; //Server Port
    	needed_arguments++; //Mode
	
	int arguments_for_rtt = needed_arguments;
	
	int arguments_for_brandwith = needed_arguments;
	arguments_for_brandwith ++; //Amount
	arguments_for_brandwith ++; //Message

	int arguments_for_packageloss = needed_arguments;
	arguments_for_packageloss ++; //Amount
	arguments_for_packageloss ++; //Message

	if(argc<needed_arguments){
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



	/* Mode auslesen */ 
       mode = stringToInt(argv[3]);
	
	if(mode<1 || mode > 3){
		myerror("Mode has to be [1..3] !");
	}
	if(mode==1){
		if(argc!=arguments_for_rtt){
			myerror("Not enough arguments for RRT!");
		}
		rtt();
	}
	if(mode==2){
		if(argc!=arguments_for_brandwith){
			myerror("Not enough arguments for RRT!");
		}
	}
	if(mode==3){
		if(argc!=arguments_for_packageloss){
			myerror("Not enough arguments for RRT!");
		}
	}



	
}
