/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * login.c: Implementierung des Logins
 */

#include "login.h"
#include "clientthread.h"
#include "user.h"
#include "common/rfc.h"
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
#include <pthread.h>


// Spielstatus verwalten
bool game_running = false;


/*
 * Login_Thread Start
 *
 * param Serversocket
 */
void* login_main(int sock){
	PACKET packet;
	packet.header.type = 0;
	packet.header.length = 0;
	int client_socket;
	int client_id;

	// Threadverwaltung - je Client ein Thread - Thread verwaltet Spielphase
	pthread_t client_threads[MAX_PLAYERS];

	// Initialisiere Userdaten-Mutex
	if(create_user_mutex() == -1){
		// sollte initialisieren nicht erfolgreich sein terminiere Thread
		pthread_exit(NULL);
	}

	// Empfaenger-Schleife
	while(1){
		// Warte auf Connection-Request
		if(listen(sock,64) == 0){
			// Bei erfolgreichem Request wird Connection aufgebaut
			client_socket = accept(sock, 0, 0);
			if(client_socket == -1){
				errorPrint("Client Socket Connection Fehler");
				exit(0);
			}

			// Verbidung hergestellt
			infoPrint("Verbindung hergestellt, warte auf Loginrequst.");

			// Empfange Paket
			packet = recvPacket(client_socket);

			// Nachricht RFC_LOGINREQUEST
			if(packet.header.type == RFC_LOGINREQUEST){

				PACKET response;

				// sofern Spiel nicht laeuft
				if(!game_running){

					// fuege Spieler zur Verwaltung hinzu, uebergebe Name + Socketdeskriptor
					client_id = addPlayer(packet.content.playername,ntohs(packet.header.length), client_socket);

					// Name bereits vorhanden
					if(client_id == -1){
						errorPrint("Name bereits vorhanden");
						response.header.type = RFC_ERRORWARNING;
						response.header.length = htons(sizeof(ERROR));
						response.content.error.errortype = ERR_SERVER_PLAYERNAMEEXIST;
						strncpy(response.content.error.errormessage, "Name bereits vorhanden", 100);
					}
					// Zu viele Spieler angemeldet
					else if(client_id >= MAX_PLAYERS){
						errorPrint("Maximale Anzahl an Spielern erreicht!");
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

                    // sofern Anmeldung ok - erstelle Clientthread fuer neu hinzugefuegten Spieler
                    if(response.header.type != RFC_ERRORWARNING){
                        // uebergebe Client-ID an Client-Thread
                        printf("Erstelle Client-Thread");
                        pthread_create(&client_threads[client_id], NULL, (void *) &client_thread_main, &client_id);
                    }

					// sende Antwort
					sendPacket(response, client_socket);

					// aktuelle Spielerliste an alle Spieler senden
					sendPlayerList();
				}
				// Spiel laeft bereits - keine Anmeldung moeglich
				else {
					errorPrint("Spiel laeft bereits, Client kann nicht angemeldet werden");
					response.header.type = RFC_ERRORWARNING;
					response.header.length = htons(sizeof(ERROR));
					response.content.error.errortype = ERR_SERVER_GAMEISRUNNING;
					strncpy(response.content.error.errormessage, "Spiel laeft bereits, Client kann nicht angemeldet werden", 100);
					// sende Antwort
					sendPacket(response, client_socket);
				}
			}
		}
	}
	// terminiere Thread
	pthread_exit(NULL);
	return NULL;
}



// setzte Spielstatus auf 'Spiel gestartet
void setGameRunning(){
	game_running = true;
	return;
}


// gebe Spielstatus zurueck
bool getGameRunning(){
	return game_running;
}
