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
#include "score.h"
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
 * Thread ist fuer die Anmeldung eines Spieler zustaendig
 *
 * param Serversocket
 */
void* login_main(int sock){
	PACKET packet;

	int client_socket;
	int client_id;

	// Threadverwaltung - je Client ein Thread - Thread verwaltet Spielphase
	pthread_t client_threads[MAX_PLAYERS];

	// Initialisiere Userdaten-Mutex
	if(create_user_mutex() == -1){
		// sollte initialisieren nicht erfolgreich sein terminiere Thread
		pthread_exit(NULL);
	}

	// Empfangs-Schleife
	while(1){
		// ankommende Verbindung annehmen
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

			// LRQ - Anmeldung eines Clients am Server
			if(isStringEqual(packet.header, "LRQ")){

				PACKET response;

				// sofern Spiel nicht laeuft
				if(!game_running){
					// fuege Spieler zur Verwaltung hinzu, uebergebe Name + Socketdeskriptor
					lock_user_mutex();
					client_id = addPlayer(packet.content.loginrequest.name,(ntohs(packet.header.length))-1, client_socket);
					unlock_user_mutex();
					// Name bereits vorhanden
					if(client_id == -1){
						char *errormsg = "Name bereits vorhanden!";
						errorPrint("Name bereits vorhanden");
						response.header.type[0] = 'E';
						response.header.type[1] = 'R';
						response.header.type[2] = 'R';
						size_t length = strlen(errormsg);
						response.header.length = htons(length+1);
						response.content.error.subtype = ERR_PLAYERNAMEEXIST;
						strncpy(response.content.error.message, errormsg , length);
					}
					// Zu viele Spieler angemeldet
					else if(client_id >= MAX_PLAYERS){
						char *errormsg = "Maximale Anzahl an Spielern erreicht!";
						errorPrint("Maximale Anzahl an Spielern erreicht!");
						response.header.type[0] = 'E';
						response.header.type[1] = 'R';
						response.header.type[2] = 'R';
						size_t length = strlen(errormsg);
						response.header.length = htons(length+1);
						response.content.error.subtype = ERR_MAXCOUNTPLAYERREACHED;
						strncpy(response.content.error.message, errormsg , length);
					}
					// LRQ - Anmeldung am Server erfolgreich
					else {
						infoPrint("Spieler erfolgreich hinzugefuegt - Client-ID: %i", client_id);
						response.header.type[0] = 'L';
						response.header.type[1] = 'O';
						response.header.type[2] = 'K';
						response.header.length = htons(2);
						response.content.loginresponseok.clientid = client_id;
						response.content.loginresponseok.RFCVersion = RFC_VERSION;
					}

                    // sofern Anmeldung ok - erstelle Clientthread fuer neu hinzugefuegten Spieler
					if(!(isStringEqual(packet.header, "ERR"))){
                        infoPrint("Erstelle Client-Thread");
                        pthread_create(&client_threads[client_id], NULL, (void *) &client_thread_main, &client_id);
                    }

					// sende Antwort
					sendPacket(response, client_socket);

					// aktuelle Spielerliste an alle Spieler senden
					//sendPlayerList();
					sem_post(&semaphor_score);
				}
				// Spiel laeft bereits - keine Anmeldung moeglich
				else {
					char *errormsg = "Spiel laeuft bereits, Client kann nicht angemeldet werden!";
					errorPrint("Spiel laeuft bereits, Client kann nicht angemeldet werden!");
					response.header.type[0] = 'E';
					response.header.type[1] = 'R';
					response.header.type[2] = 'R';
					size_t length = strlen(errormsg);
					response.header.length = htons(length+1);
					response.content.error.subtype = ERR_GAMEISRUNNING;
					strncpy(response.content.error.message, errormsg , length);
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



/*
 * Funktion setzt den  Spielstatus auf 'Spiel gestartet
 */
void setGameRunning(){
	game_running = true;
	return;
}


/*
 * Funktion gibt den  Spielstatus zurueck
 */
bool getGameRunning(){
	return game_running;
}
