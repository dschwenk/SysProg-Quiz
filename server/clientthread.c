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




void *client_thread_main(int* client_id) {

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


	PACKET packet;

	// Empfangsschleife
	while(1){
		packet = recvPacket(spieler.sockDesc);

		// werte empfangenes Paket aus
		switch(packet.header.type){
			// Fehlernachricht empfangen - RFC_ERRORWARNING
			case RFC_ERRORWARNING:

				// pruefe Subtyp
				// Spieler hat das Spiel verlassen
				if(packet.content.error.errortype == ERR_CLIENT_CLIENTLEFTGAME){
					// pruefe ob Spielleiter
					// TODO
					if(is_spielleiter){

					}
					// Spieler war kein Spieleiter
					else {

					}
				}
				pthread_exit(0);
				return NULL;

			// Client hat Kataloge angefagt - RFC_CATALOGREQUEST
			case RFC_CATALOGREQUEST:
				debugPrint("Catalog Request von Spieler-ID: %i Name: %s", spieler.id, &spieler.name);
				lock_user_mutex();
				sendCatalog(spieler.sockDesc);
				// pruefe ob Katalog ausgewaehlt wurde - falls ja sende Katalog
				// TODO
				// ??
				if(isCatalogChosen()){
					// SendCatalogChange();
				}
				unlock_user_mutex();
				break;
		}
	}



	pthread_exit(0);
	return NULL;
}

int Timer(uint8_t* selection, int timeout, int sockD) {

	return 1;

}

