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


void receiveErrorMessage (PACKET packet){
	char message[(ntohs (packet.header.length))];
	// hole Errornachricht
	strncpy (message, packet.content.error.errormessage, ntohs (packet.header.length)-1);
	// Nullterminierung
	message[ntohs (packet.header.length)-1]= '\0';
	// zeige Errordialog + gebe Fehler auf Konsole aus
	printf("Fehler: %s\n", packet.content.error.errormessage);
	guiShowErrorDialog(message, packet.content.error.errortype);
	if(packet.content.error.errortype==1){
		exit(0);
	}
}



void *listener_main(int * sockD){
	int stop = 0;
	while(stop==0){
		PACKET packet = recvPacket(*sockD);
		switch (packet.header.type){
			// RFC_PLAYERLIST
			case 6:
				receivePlayerlist(packet);
				break;
			// RFC_ERRORWARNING
			case 255:
				receiveErrorMessage(packet);
				break;
			default:
				break;
		}
	}
	return 0;
}
