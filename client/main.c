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
#include "listener.h"
#include "fragewechsel.h"

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
#include <pthread.h>
#include <printf.h>


int socketDeskriptor;
int clientID;


char *name = "unknown";
// Standardserver
char *server = "localhost";
// Standardserverport
char *port = "8111";
// Ausgewählte Antwort
int selection = 0;

int establishConnection(int socketD_, char* port_, char* hostname_) {
	int portno;
	struct sockaddr_in serv_addr;
	struct hostent * server;
	int failure = 0;

	portno = atoi(port_);
	server = gethostbyname(hostname_);
	if(server == NULL ){
		infoPrint("Error, no such host");
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

void catalogRequest() {
    PACKET packet;
    packet.header.type = RFC_CATALOGREQUEST;
    packet.header.length = htons(0);
    // sende Nachricht
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


// werte Kommandozeilenparameter aus
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
    	char opt = getopt_long(argc, argv, short_options, long_options, &opt_index);
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
}



int main(int argc, char **argv){

	setProgName(argv[0]);

	// debugEnable();

	// Parameter auswerten
	process_client_commands(argc, argv);

	//Socket anlegen
	socketDeskriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// stelle Verbindung zum Server her
	if(establishConnection(socketDeskriptor, port, server) == 0){

		infoPrint("Verbindung hergestellt\n");

		// GUI initialisieren
		infoPrint("Initialisiere GUI\n");
		guiInit(&argc, &argv);

		// Erstellen des LoginRequest
		infoPrint("sende LoginReqest\n");
		loginRequest(name);

		// strate Listener_thread
		pthread_t Listener_thread;
		pthread_create(&Listener_thread, NULL, (void *) &listener_main,&socketDeskriptor);

		//Fragewechsel-Thread erzeugen
		sem_init(&frage, 0, 0);
		pthread_t Fragewechsel_thread;
		pthread_create(&Fragewechsel_thread, NULL, (void *) &fragewechsel_main,	&socketDeskriptor);

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
    PACKET packet;
    packet.header.type = RFC_CATALOGCHANGE;
    packet.header.length = htons(strlen(newSelection));
    strncpy(packet.content.catalogname, newSelection, strlen(newSelection));
    sendPacket(packet, socketDeskriptor);
    fflush(stdout);
}


void preparation_onStartClicked(const char *currentSelection) {
    debugPrint("Starte Katalog %s\n", currentSelection);
    PACKET packet;
    packet.header.type = RFC_STARTGAME;
    packet.header.length = htons(strlen(currentSelection));
    //preparation_hideWindow();
    strncpy(packet.content.catalogname, currentSelection, strlen(currentSelection));
    sendPacket(packet, socketDeskriptor);
}


void preparation_onWindowClosed(void) {
	debugPrint("Vorbereitungsfenster geschlossen");
    PACKET packet;
    packet.header.type = RFC_ERRORWARNING;
    packet.header.length = htons(strlen("Der Spieler hat das Spiel verlassen!"));
    packet.content.error.errortype = ERR_CLIENT_CLIENTLEFTGAME;
    strncpy(packet.content.error.errormessage,"Der Spieler hat das Spiel verlassen!",ntohs(packet.header.length));
    sendPacket(packet, socketDeskriptor);
    guiQuit();
}


void game_onSubmitClicked(unsigned char selectedAnswers)
{
	infoPrint("Absende-Button angeklickt, Bitmaske der Antworten: %u",	(unsigned)selectedAnswers);

	selection = selectedAnswers;
	PACKET packet;

	packet.header.type = RFC_QUESTIONANSWERED;
	packet.header.length = htons(sizeof(uint8_t));
	packet.content.selection = (uint8_t) selection;
	printf("blaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa %d", selection);
	sendPacket(packet, socketDeskriptor);
}


void game_onWindowClosed(void) {
	debugPrint("Spielfenster geschlossen");
    PACKET packet;
    packet.header.type = RFC_ERRORWARNING;
    packet.header.length = htons(strlen("Der Spieler hat das Spiel verlassen!"));
    packet.content.error.errortype = ERR_CLIENT_CLIENTLEFTGAME;
    strncpy(packet.content.error.errormessage,"Der Spieler hat das Spiel verlassen!",ntohs(packet.header.length));
    sendPacket(packet, socketDeskriptor);
    guiQuit();
}
