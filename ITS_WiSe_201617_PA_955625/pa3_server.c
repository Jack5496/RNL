#include <signal.h> /* Damit ich Signale abfangen kann */
#include <unistd.h> /* für strg c anfangen */

#include <stdio.h> /* Für standard Dinge */
#include <stdlib.h> /* malloc */
#include <string.h> /* für strings */
#include <sys/socket.h> /* sockets */
#include <netinet/in.h>
#include <arpa/inet.h>

#include <gpgme.h>   /* gpgme             */
#include <errno.h>   /* errno             */
#include <locale.h>  /* locale support    */

int server_port = 5050; /* default port */
int keep_alive = 1; /* boolean solange zugehört wird */
int data_length;  /* länge der ankommenden daten */
char* buffer;  /* Buffer für ankommende Daten */
char* bufferPointer;  /* Buffer für ankommende Daten */
char server_adress[] = "127.0.0.1";  /* Default server adresse */
int udpSocket;  /* Udp Socket */

gpgme_ctx_t ctx;  /* Context für GPGME */
gpgme_error_t err;  /* Error Code */
gpgme_data_t in;  /* Data Input */
gpgme_data_t plain;  /* Plaintext of Message */
gpgme_verify_result_t verify_result;  /* The verify of the Signed Message */
gpgme_signature_t sig;  /* The signature */

/**
* Hiermit wird unser GPG initialisiert
*/
void gpgInit(){
	/* Begin setup of GPGME */
	gpgme_check_version (NULL); /* Entnommen aus der Doku */
	setlocale (LC_ALL, "");
	gpgme_set_locale (NULL, LC_CTYPE, setlocale (LC_CTYPE, NULL));
	/* End setup of GPGME */

	err = gpgme_engine_check_version (GPGME_PROTOCOL_GPGCONF);
	if(err){
		printf("GPGME: Engine Check failed\n");
		keep_alive = 0;
		return;
	}

	err = gpgme_new (&ctx);  /* Erstellen des neuen Context */
	if(err){
		printf("GPGME: Context creation failed\n");
		keep_alive = 0;
		return;
	}
}

/**
* Hierdurch beenden wir unser GPG
*/
void gpgRelease(){
	gpgme_release (ctx);
}

/**
* Letzer aufruf um alles wichtige zu schließen
*/
void last_wish(int i){
	printf("\nManuelles Beenden\n");
	keep_alive=0; /* Wir möchten unsere While Loop beenden */
}

void gpgCheckSign() {
	 /* Erstelle Data in, welches unseren text beinhaltet */
	err = gpgme_data_new_from_mem (&in, buffer, data_length, 1);
	if(err){
		printf("GPGME: Read Buffer into In failed\n");
		keep_alive = 0;
		return;
	}

	 /* Erstelle Data Plain, welches den Plaintext enthalten soll */
	err = gpgme_data_new (&plain);
	if(err){
		printf("GPGME: Data new plain failed\n");
		keep_alive = 0;
		gpgme_data_release (in);
		return;
	}
	
	 /* Führe ein Verify aus */
	err = gpgme_op_verify (ctx, in,NULL,plain);
	if(err){
		printf("GPGME: Verify failed\n");
		gpgme_data_release (plain);
		gpgme_data_release (in);
		keep_alive = 0;
		return;
	}
	
	 /* Hole das Ergebniss des Verify */
	verify_result = gpgme_op_verify_result (ctx);

	 /* Fahre fort, nur wenn wir was haben und dies Signaturen besitzt */
	if (verify_result && verify_result->signatures){
		 /* Fahre fort, nur wenn wir bei dem Status keinen Fehler haben */
		if(gpg_err_code(verify_result->signatures->status)==GPG_ERR_NO_ERROR){
			gpgme_key_t key;  /* Erstelle neuen halter unseres Keys */
			
			 /* Fülle nun den Key */
			err = gpgme_get_key (ctx, verify_result->signatures->fpr, &key, 0);
			if(err){
				printf("GPGME: get Key failed\n");
				gpgme_data_release (plain);
				gpgme_data_release (in);
				keep_alive = 0;
				return;
			}

			 /* AUSGABE: Namen des Senders falls vorhanden */
			if(key->uids->name){
				printf("%s: ",key->uids->name);
			}
			else{
				printf("GPGME: No Name in Key UIDS\n");
				gpgme_data_release (plain);
				gpgme_data_release (in);
				keep_alive = 0;
				return;
			}
			gpgme_key_release (key); /* Release den Key */
				
			/* Erstelle halter der Länge des Plaintext */
			size_t plainTextLength;
			
			err = gpgme_data_seek(plain,0,SEEK_SET); /* Setzte Pointer zum anfang*/
			if(err){
				printf("GPGME: Data Seek Failed\n");
				gpgme_data_release (plain);
				gpgme_data_release (in);
				keep_alive = 0;
				return;
			}
			
			/* Hole den Plaintext */
			char* plainText = gpgme_data_release_and_get_mem(plain,&plainTextLength);

			/* Printe jeden einzelnen Char hinter den Namen */
			int i;
			for(i=0; i<plainTextLength; i++){
				printf("%c",plainText[i]);	
			}


		}
		else{
		printf("Die Signatur ist INVALID\n");
		}
	}
	else{
		printf("GPGME: Verify result failed\n");
		gpgme_data_release (plain);
		gpgme_data_release (in);
		keep_alive = 0;
		return;
	}
	
	gpgme_data_release (in);
	
}


int main(int argc, char **argv){
	//Handlet aktivierung für STRG+C
	//http://stackoverflow.com/questions/1641182/how-can-i-catch-a-ctrl-c-event-c
	struct sigaction sigIntHandler;
	int i;

	sigIntHandler.sa_handler = last_wish;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0; //setze sa flags 0

	sigaction(SIGINT, &sigIntHandler, NULL);
	// Ende für STRG+C


	int needed_arguments = 1; //programm selber
	needed_arguments++; //Server Port

	/* Starte GPGME */ 
	gpgInit();

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
			gpgRelease();
			return 1;
		}        
		server_port = atoi(argv[1]);

		//Einfacher UDP Server http://www.programminglogic.com/sockets-programming-in-c-using-udp-datagrams/

		/* Erstelle Alles für das aufsetzen des Sockets */
		struct sockaddr_in serverAddr;
		struct sockaddr serverStorage;
		socklen_t addr_size;

		/* Create UDP socket */
		udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if(udpSocket<0){
			printf("Unable to setup Socket\n");	
			keep_alive = 0;
		}

		/*Configure settings in address struct*/
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(server_port);
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		/*Bind socket with address struct*/
		int binding = bind(udpSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
		if(binding<0){
			printf("Binding doesnt worked\n");
			keep_alive = 0;
		}


		/*Initialize size variable to be used later on*/
		addr_size = sizeof serverStorage;

		/* Erstelle genug Platz für ankommende Nachrichten */
		buffer = malloc(sizeof(char)*65536);
		bufferPointer = buffer;

		if(keep_alive){
			printf("Server starting: %s:%i \n\n",server_adress,server_port);  	  
		}
		/* Solange wir aktiv sein sollen */
		while(keep_alive){

			/* Hole alle ankommenden Daten und speichere diese in Buffer zwischen */
			data_length = recvfrom(udpSocket,buffer,65536,0,(struct sockaddr *)&serverStorage, &addr_size);

			/* Falls wir was erhalten haben */
			if(data_length>=0){	  
				gpgCheckSign();
			}
			else{
				if(keep_alive!=0){ //falls kein selbstgewollter Abbruch
				printf("Recieve From Error\n");
				}
				break;
			}
		}

		/* Falls wir unseren Socket noch schließen müssen, tun wir das */
		if(udpSocket > 0){
			close(udpSocket);
		}
		/* Wir freen mal alles was noch offen ist */
		free(bufferPointer);
		gpgRelease();
	}
	else{
		printf("usage: ./pa3_server PORT \n");
	}

	return 0;
}
