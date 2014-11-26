/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * clientthread.c: Implementierung des Client-Threads
 */

#include "common/util.h"
#include "common/rfc.h"
#include "common/networking.h"
#include "user.h"
#include "main.h"
#include "login.h"
#include "score.h"
#include "catalog.h"

#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "catalog.h"
#include <time.h>
#include <stdio.h>


void *client_thread_main(int* client_id) {

    printf("Entry point client-thread");

	bool is_spielleiter = false;

	// hole Spielerinformationen
	PLAYER spieler;
	lock_user_mutex();
	spieler = getUser(*client_id);
	unlock_user_mutex();

	// ist der Spieler Spielleiter
	if(spieler.id == 0){
		is_spielleiter = true;
	}

	// Struktur zum empfangen + senden von Nachrichten
	PACKET packet, response;

	// Empfangsschleife
	while(1){
        printf("entry while");

		packet = recvPacket(spieler.sockDesc);

        printf("clientthread packet: %d", packet.header.type);

		// werte empfangenes Paket aus
		switch(packet.header.type){
			// Fehlernachricht empfangen - RFC_ERRORWARNING
			case RFC_ERRORWARNING:
				// pruefe Subtyp
				// Spieler hat das Spiel verlassen
				if(packet.content.error.errortype == ERR_CLIENT_CLIENTLEFTGAME){
					debugPrint("Spieler %s (ID: %d) hat das Spiel verlassen", spieler.name, spieler.id);
					// pruefe ob Spielleiter
					if(is_spielleiter){
						debugPrint("Spieler %s (ID: %d) war Spielleiter", spieler.name, spieler.id);
						infoPrint("Der Spieleiter hat das Spiel verlassen, der Server wird beendet");
						// setzte Fehlertyp + Text
						response.header.type = RFC_ERRORWARNING;
						response.header.length = htons(strlen("Der Spieleiter hat das Spiel verlassen, der Server wird beendet!"));
						response.content.error.errortype = ERR_SERVER_SPIELLEITERLEFTGAME;
						strncpy(response.content.error.errormessage,"Der Spieleiter hat das Spiel verlassen, der Server wird beendet!",	ntohs(response.header.length));
						//lock_user_mutex();
						// sende Fehlermeldung an alle
						sendToAll(response);
						//unlock_user_mutex();
						// Server beenden
						endServer();
					}
					// Spieler war kein Spieleiter
					else {
						debugPrint("Spieler %s (ID: %d) war NICHT Spielleiter", spieler.name, spieler.id);
						// entferne Spieler aus verwaltung
						//lock_user_mutex();
						removePlayer(spieler.id);
						// pruefe ob Spiel bereits laeft und Anzahl verbliebener Spieler
						if((getGameRunning()) && (countUser() <= 2)){
							// zu wenig Spieler
							response.header.type = RFC_ERRORWARNING;
							response.header.length = htons(strlen("Zu wenig Spieler, breche Spiel ab."));
							response.content.error.errortype = ERR_SERVER_TOOFEWPLAERS;
							strncpy(response.content.error.errormessage,"Zu wenig Spieler, breche Spiel ab.",ntohs(response.header.length));
							// sende Fehlermeldung an alle
							sendToAll(response);
							//unlock_user_mutex();
							// Server beenden
							// endServer();
						}
						// sende Spielerliste an alle verbliebene Spieler
						sendPlayerList();
						//unlock_user_mutex();
					}
				}
				// beende Thread
				pthread_exit(0);
				return NULL;

				// Client hat Kataloge angefagt - RFC_CATALOGREQUEST
				case RFC_CATALOGREQUEST:
					debugPrint("Catalog Request von Spieler-ID: %i Name: %s", spieler.id, spieler.name);
					//lock_user_mutex();
					sendCatalog(spieler.sockDesc);
					// pruefe ob Katalog ausgewaehlt wurde - falls ja sende Katalog
					if(isCatalogChosen()){
						// Client erkennt somit den vom Spielleiter ausgewaehlten Katalog
						sendCatalogChange();
					}
					//unlock_user_mutex();
					break;

				// Client hat Katalog gewechselt
				case RFC_CATALOGCHANGE:
					debugPrint("Catalog Change von Spieler-ID: %i Name: %s", spieler.id, spieler.name);
					// setzte aktiven Katalog
					setActiveCatalog(packet);
					// sende Katalogwechsel an alle Spieler
					sendCatalogChange();
					break;
				case RFC_STARTGAME:
					// pruefe Anzahl angemelder Spieler - >= 2 Spieler zum Spielstart benoetigt
					// zu wenig Spieler - Spiel wird nicht gestartet
					if(countUser() < 2){
						infoPrint("Zu wenige Spieler um das Spiel zu starten!");
						response.header.type = RFC_ERRORWARNING;
						response.header.length = htons(strlen("Zu wenige Spieler um das Spiel zu starten!"));
						response.content.error.errortype = ERR_SERVER_COULDNOTSTARTGAME;
						strncpy(response.content.error.errormessage,"Zu wenige Spieler um das Spiel zu starten!", ntohs(response.header.length));
						sendPacket(response, spieler.sockDesc);
						// SendToAll(response);
					}
					// Spiel wird gestartet
					else {
						setGameRunning();
						char catalog[31];
						strncpy(catalog, packet.content.catalogname,ntohs(packet.header.length));
						catalog[ntohs(packet.header.length)] = '\0';
						// LoaderQuestions(kat);
						// QNumber = 0;
						// user_mutex_lock();
						// sende Paket mit aktuellem Katalog an alle Spieler
						sendToAll(packet);
						// user_mutex_unlock();
					}
					break;
				case RFC_QUESTIONREQUEST:
					// ....

				// unbekannter Nachrichtentyp
				default:
					debugPrint("Unbekannter Nachrichtentyp: %i von Spieler-ID: %i Name: %s", packet.header.type, spieler.id, spieler.name);
					break;
		}
	}
	pthread_exit(0);
	return NULL;
}
