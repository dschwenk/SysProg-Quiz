/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Gemeinsam verwendete Module
 *
 * netwokring.c: Implementierung der Funktionen zum Senden & Empfangen von Nachrichten
 *
 */

#include "networking.h"
#include "sockets.h"
#include "common/util.h"
#include "rfc.h"
#include "util.h"
#include "../client/gui/gui_interface.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>



/*
 * versende Nachricht ueber Socket
 * param PACKET packet Nachricht die versendet werden soll
 * int socketDeskriptor Socketdeskriptor ueber den die Nachricht versendet wird
 */
void sendPacket(PACKET packet, int socketDeskriptor){
	// sende Daten ueber Socket
	if(send(socketDeskriptor, &packet, ntohs(packet.header.length)+3,0) == -1){
		errorPrint("Senden der Daten fehlgeschlagen!");
		exit(0);
	}
	else {
		// Testweise ausgeben welcher Typ an welchen Socket versendet wurde
		printf("Nachicht vom Type %i an die Socket-ID: %i gesendet\n", packet.header.type, socketDeskriptor);
	}

}


/*
 * Nachricht ueber Socket empfangen
 * param int socketDeskriptor Socketdeskriptor ueber den Nachricht empfangen wird *
 */
PACKET recvPacket (int socketDeskriptor){

	PACKET packet;
	int recv_bytes = 0;

	// empfange Header
	// stimmt die Anzahl an empfangen Daten mit der Groese des Headers ueberrein?
	if((recv_bytes = recv(socketDeskriptor, &packet.header, sizeof(packet.header), MSG_WAITALL)) != sizeof(packet.header)){
		errorPrint("Fehlerhafter Header empfangen. Anzahl empfangener Bytes: %d ", recv_bytes);
		// gebe Fehlerpaket zurueck
		// setzte Header + Errortype + Fehlernachricht
		char error_message[] = "Fehlerhafte uebertragung der Daten, Verbindung unterbrochen!";
		packet.header.type = RFC_ERRORWARNING;
		packet.content.error.errortype = ERR_FATAL;
		strcpy(packet.content.error.errormessage, error_message);
		packet.header.length = htons(3 + strlen(error_message) + 1);
		return packet;
	}
	// empfange Content
	if(htons(packet.header.length)){
		// stimmt die Anzahl an empfangen Daten mit der Groesenangabe im Paket ueberrein?
		if((recv_bytes = recv(socketDeskriptor, &packet.content, htons(packet.header.length), MSG_WAITALL)) != htons(packet.header.length)){
			errorPrint("Fehlerhaftes Datenpaket empfangen. Anzahl empfangener Bytes: %d ", recv_bytes);
			// gebe Fehlerpaket  zurueck
			// setzte Header + Errortype + Fehlernachricht
			char error_message[] = "Fehlerhaftes Datenpacket";
			packet.header.type = RFC_ERRORWARNING;
			packet.content.error.errortype = ERR_FATAL;
			strcpy(packet.content.error.errormessage, error_message);
			packet.header.length = htons(3 + strlen(error_message) + 1);
			return packet;
		}
	}
	// empfangen war erfolgreich, gebe Packet zurueck
	return packet;
}




