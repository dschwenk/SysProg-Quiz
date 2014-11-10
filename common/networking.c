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



// packetSchreiben
void sendPacket(PACKET packet, int socketDeskriptor){
	// Laenge
	send(socketDeskriptor, &packet, ntohs(packet.header.length)+3,0);
	// Testweise ausgeben welcher Typ an welchen Socket versendet wurde
	printf("Ein Packet vom Type %i wurde an die SockID: %i versandt\n", packet.header.type, socketDeskriptor);
}


PACKET recvPacket (int socketDeskriptor ){

	// Paket, initialisiere mit Header + Length mit 0
	PACKET packet;
	packet.header.type = 0;
	packet.header.length = 0;

	int readresult = (int)read(socketDeskriptor,&(packet.header),3);

	//Socket kann nicht gelesen werden, Fehler
	if(readresult == 0){
		packet.header.type = 0;
	}
	//Socket kann gelesen werden, Übergabe vom content und Länge
	else {
		recv(socketDeskriptor, &(packet.content),ntohs(packet.header.length),0);
	}

	return packet;
}




