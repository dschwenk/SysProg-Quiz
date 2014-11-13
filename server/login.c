/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * login.c: Implementierung des Logins
 */

#include "login.h"
#include "common/rfc.h"
#include "user.h"
#include "common/sockets.h"
#include "common/networking.h"

#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


/*
 * Login_Thread Start
 *
 */

void* login_main(int sock) {
	PACKET packet;
	packet.header.type = 0;
	packet.header.length = 0;
	int client_socket;
	int client_id;

	// Empfaenger-Schleife
	while(1){
		// Warte auf Connection-Request
		if(listen(sock,64) == 0){
			// Bei erfolgreichem Request wird Connection aufgebaut
			client_socket = accept(sock, 0, 0);
			if(client_socket == -1){
				perror("Client Socket Connection Fehler");
				exit(0);
			}

			// Verbidung hergestellt
			infoPrint("Verbindung hergestellt, warte auf Loginrequst.");

			// Empfange Paket
			packet = recvPacket(client_socket);

			// Nachricht RFC_LOGINREQUEST
			if(packet.header.type == 1){
				PACKET response;
				client_id = addPlayer(packet.content.playername,ntohs(packet.header.length), client_socket);

				// Name bereits vorhanden
				if(client_id == -1){
					perror("Name bereits vorhanden");
					response.header.type = RFC_ERRORWARNING;
					response.header.length = htons(sizeof(ERROR));
					response.content.error.errortype = ERR_SERVER_PLAYERNAMEEXIST;
					strncpy(response.content.error.errormessage, "Name bereits vorhanden", 100);
				}
				// Zu viele Spieler angemeldet
				else if(client_id >= 4){
					perror("Maximale Anzahl an Spielern erreicht!");
					response.header.type = RFC_ERRORWARNING;
					response.header.length = htons(sizeof(ERROR));
					response.content.error.errortype = ERR_SERVER_MAXCOUNTPLAYERREACHED;
					strncpy(response.content.error.errormessage, "Maximale Anzahl an Spielern erreicht!", 100);
				}
				// ID ok - RFC_LOGINRESPONSEOK
				else {
					infoPrint("Spieler erfolgreich hinzugefuegt - Client-ID: %i", client_id);
					response.header.type = RFC_LOGINRESPONSEOK;
					response.header.length = htons(sizeof(uint8_t));
					response.content.clientid = client_id;
				}

				// sende Antwort
				sendPacket(response, client_socket);
				// PLAYERLIST an alle Senden
				sendPlayerList();
			}
		}
	}
	pthread_exit(NULL);
	return NULL;
}
