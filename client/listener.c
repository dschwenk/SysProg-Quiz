/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Client
 * 
 * listener.c: Implementierung des Listener-Threads
 */

#include "listener.h"
#include "gui/gui_interface.h"
#include "common/rfc.h"
#include "common/sockets.h"
#include "common/networking.h"
#include "main.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>



void receivePlayerlist(PACKET packet){

	USER userlist[4];
	int spielerzahl=0;

	// printf("Test: %i\n",packet.header.length);
	spielerzahl = ntohs(packet.header.length)/37;
	printf("Anzahl Spieler in der Playerlist: %i\n",spielerzahl);
	// Playerlist leeren
	preparation_clearPlayers();
	for (int i =0; i< spielerzahl;i++){
		// kopiere Spieler ID in Spielerliste
		userlist[i].id=packet.content.playerlist[i].id;
		// mehr als 4 Spieler?
		if(i > 4){
			printf("Maximale Anzahl an Spieler erreicht!\n");
			exit(1);
		}
		// kopiere Name aus Paket in Spielerliste
		strncpy(userlist[i].name, packet.content.playerlist[i].playername,32);
		// Ausgabe der angemeldeten Spieler
		printf("%s ist angemeldet\n", userlist[i].name);
		preparation_addPlayer(userlist[packet.content.playerlist[i].id].name);
	}
}

void receiveCatalogList(PACKET packet) {
    // Antwort auswerten und anzeigen
    infoPrint("%s", packet.content.catalogname);
    preparation_addCatalog(packet.content.catalogname);
}

void receiveErrorMessage (PACKET packet){
	char error_message[100];
	// hole Errornachricht
	strncpy (error_message, packet.content.error.errormessage, ntohs (packet.header.length)-1);
	// Nullterminierung
	error_message[ntohs (packet.header.length)-1]= '\0';
	// zeige Errordialog + gebe Fehler auf Konsole aus
	errorPrint("Fehler: %s\n", packet.content.error.errormessage);
	// zeige Fehler in GUI
	// guiShowMessageDialog(error_message, packet.content.error.errortype);
	guiShowErrorDialog(error_message, packet.content.error.errortype);
	// beende Client falls fataler Error
	if((packet.content.error.errortype == ERR_SERVER_SPIELLEITERLEFTGAME) || (packet.content.error.errortype == ERR_SERVER_TOOFEWPLAERS)){
		exit(0);
	}
}


void *listener_main(int * sockD){

    // warte auf Antwort des Servers
    PACKET response = recvPacket(*sockD);
    int clientID;

    // RFC_LOGINRESPONSEOK
    if(response.header.type == RFC_LOGINRESPONSEOK){
        infoPrint("Login Response ok\n");
        clientID = response.content.clientid;

        // RFC_CATALOGREQUEST - get catalog list
        catalogRequest();
    }
        // RFC_ERRORWARNING
    else if(response.header.type == RFC_ERRORWARNING){
        char message[(ntohs(response.header.length))];
        strncpy(message, response.content.error.errormessage,ntohs(response.header.length) - 1);
        // Nullterminierung
        message[ntohs(response.header.length) - 1] = '\0';
        // zeige in GUI Fehlermeldung an
        infoPrint("Fehler: %s\n", message);
        guiShowErrorDialog(message, response.content.error.errortype);
        exit(0);
    }
        // Verbindung verloren
    else {
        // zeige in GUI Fehlermeldung an
        infoPrint("Verbindung zum Server verloren!");
        guiShowErrorDialog("Verbindung zum Server verloren!", response.content.error.errortype);
        exit(0);
    }

    //An GUI mitteilen ob Spielleiter oder nicht
    if(clientID == 0){
        preparation_setMode(PREPARATION_MODE_PRIVILEGED);
    }
    else {
        preparation_setMode(PREPARATION_MODE_NORMAL);
    }

    // Empfangsschleife
	int stop = 0;
	while(stop==0){
		PACKET packet = recvPacket(*sockD);
		switch (packet.header.type){
            // RFC_CATALOGRESPONSE
            case RFC_CATALOGRESPONSE:
                receiveCatalogList(packet);
                break;
            // RFC_CATALOGCHANGE
            case RFC_CATALOGCHANGE:
                preparation_selectCatalog(packet.content.catalogname);
                break;
			// RFC_PLAYERLIST
			case RFC_PLAYERLIST:
				receivePlayerlist(packet);
				break;
			// RFC_STARTGAME
			case RFC_STARTGAME:
				// Vorbereitungsfenster ausblenden und Spielfenster anzeigen
				game_showWindow();
				preparation_hideWindow();
				break;
			// RFC_ERRORWARNING
			case RFC_ERRORWARNING:
				receiveErrorMessage(packet);
				break;
			default:
				break;
		}
	}
	return 0;
}
