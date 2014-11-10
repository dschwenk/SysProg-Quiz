/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Client
 * 
 * main.c: Hauptprogramm des Clients
 */

#include "common/util.h"
#include "gui/gui_interface.h"
#include "common/sockets.h"
#include "common/networking.h"
#include "common/rfc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <getopt.h>
#include <stdbool.h>
#include "listener.h"


int socketDeskriptor;
int clientID;


char *name = "unknown";
// Standardserver
char *server = "localhost";
// Standardserverport
char *port = "8111";


int verbindungAufbauen(int socketD_, char* port_, char* hostname_) {
	int portno;
	struct sockaddr_in serv_addr;
	struct hostent * server;
	int failure = 0;

	portno = atoi(port_);
	server = gethostbyname(hostname_);
	if(server == NULL ){
		perror("Error, no such host");
		failure = 1;
	}
	else {
		bzero((char*) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		bcopy(server->h_addr, &serv_addr.sin_addr,server->h_length);
		serv_addr.sin_port = htons(portno);

		if(connect(socketD_, (struct sockaddr*) &serv_addr, sizeof(serv_addr))	< 0){
			infoPrint("Error: no such host");
			failure = 1;
		}
	}
	return failure;
}


void loginRequest(char* name) {
	PACKET packet;
	packet.header.type = RFC_LOGINREQUEST;
	// Laenge Name berechnen + Name in packet kopieren
	packet.header.length = htons(strlen(name));
	strncpy(packet.content.playername, name, strlen(name));
	// sende Nachrichtc
	sendPacket(packet, socketDeskriptor);
}


// gebe Hilfe aus
void show_Clienthelp() {
    printf("Available options:\n");
    printf("    -h --help       show this help message\n");
    printf("    -n --name       specify a name (argument)\n");
    printf("    -v --verbose    enable debug output\n");
    printf("    -p --port       specify a port (argument)\n");
    printf("    -s --server     specify a server (argument)\n");
}


void process_client_commands(int argc, char** argv) {

    debugPrint("Parsing command line options...");

	bool userNameIsSet = false;

    const char* short_options = "hn:vp:s:";
    struct option long_options[] = {
        { "help", no_argument, 0, 'h' },
        { "name", required_argument, 0, 'n' },
        { "verbose", no_argument, 0, 'v' },
        { "port", optional_argument, 0, 'p' },
        { "server", optional_argument, 0, 's' },
        { NULL, 0, NULL, 0 }
    };

    int opt_index = 0;
    int loop = 1;
    while(loop != 0){
    	int opt = getopt_long(argc, argv, short_options, long_options, &opt_index);
    	switch(opt){
			// zeige Hilfe
    		case 'h':
    			show_Clienthelp();
				exit(1);
				break;
			// Name
			case 'n':
				name = optarg;
				if(strlen(name) >= 32) {
				    infoPrint("Name darf nur max. 32 Zeichen lang sein");
				    exit(1);
				}
				userNameIsSet = true;
				break;
			// Verbose
			case 'v':
				debugEnable();
		    	infoPrint("debug Ausgabe aktiviert.\n");
				break;
			// Port
			case 'p':
				port = optarg;
				break;
			// Server
			case 's':
				server = optarg;
				break;
			case -1:
				loop = 0;
				break;
			// Option nicht bekannt
			default:
				break;
			}
    	}

	// Es wurde kein Name angegeben
	if(!userNameIsSet){
    	infoPrint("Es wurde kein Name angegeben\n");
    	show_Clienthelp();
		exit(0);
	}

	// Name zu lang
	if(strlen(name) > 31){
    	infoPrint("Der Name ist zu lang\n");
		exit(0);
	}
}



int main(int argc, char **argv){

	setProgName(argv[0]);

	// debugEnable();

	// Parameter auswerten
	process_client_commands(argc, argv);

	//Socket anlegen
	socketDeskriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// baue Verbindung auf
	if(verbindungAufbauen(socketDeskriptor, port, server) == 0){

    	infoPrint("Verbindung aufgebaut\n");

		// GUI initialisieren
    	infoPrint("Initialisiere GUI\n");
    	guiInit(&argc, &argv);

		// Erstellen des LoginRequest
    	infoPrint("sende LoginReqest\n");
    	loginRequest(name);

		// warte auf Antwort des Servers
		PACKET response = recvPacket(socketDeskriptor);

		// RFC_LOGINRESPONSEOK
		if(response.header.type == 2){
	    	infoPrint("Login Response ok\n");
			//printf("ClientID: %i\n",response.content.clientid);
			clientID = response.content.clientid;
		}
		// RFC_ERRORWARNING
		else if(response.header.type == 255){
			char message[(ntohs(response.header.length))];
			strncpy(message, response.content.error.errormessage,ntohs(response.header.length) - 1);
			// Nullterminierung
			message[ntohs(response.header.length) - 1] = '\0';
			// zeige in GUI Fehlermeldung an
			guiShowErrorDialog(message, response.content.error.errortype);
			exit(0);
		}
		// Verbindung verloren
		else {
			// zeige in GUI Fehlermeldung an
			guiShowErrorDialog("Verbindung zum Server verloren!",
			response.content.error.errortype);
			exit(0);
		}

		// strate Listener_thread
		pthread_t Listener_thread;
		pthread_create(&Listener_thread, NULL, (void *) &listener_main,&socketDeskriptor);

		//An GUI mitteilen ob Spielleiter oder nicht
		if(clientID == 0){
			preparation_setMode(PREPARATION_MODE_PRIVILEGED);
		}
		else {
			preparation_setMode(PREPARATION_MODE_NORMAL);
		}

		//GUI Anzeigen
		preparation_showWindow();
		guiMain();

		// schliesse Gui
		guiDestroy();
	}

	//Socket schließen
	close(socketDeskriptor);
	return 0;
}




void preparation_onCatalogChanged(const char *newSelection) {
	debugPrint("Katalogauswahl: %s", newSelection);
}

void preparation_onStartClicked(const char *currentSelection) {
	debugPrint("Starte Katalog %s", currentSelection);
}

void preparation_onWindowClosed(void) {
	debugPrint("Vorbereitungsfenster geschlossen");
	guiQuit();
}

void game_onAnswerClicked(int index) {
	debugPrint("Antwort %i ausgewählt", index);
}


void game_onSubmitClicked(unsigned char selectedAnswers)
{
	debugPrint("Absende-Button angeklickt, Bitmaske der Antworten: %u",	(unsigned)selectedAnswers);
}


void game_onWindowClosed(void) {
	debugPrint("Spielfenster geschlossen");
	guiQuit();
}
