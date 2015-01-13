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
#include "fragewechsel.h"
#include "../common/rfc.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdbool.h>
#include <pthread.h>


int clientID;
bool game_is_running = false;


/*
 * Funktion wertet spielerliste aus und aktualisiert
 * die Spielerliste in der GUI
 */
void receivePlayerlist(PACKET packet){

	USER userlist[MAX_PLAYERS];
	int spielerzahl=0;

	spielerzahl = ntohs(packet.header.length)/37; // 37 == Groesse PLAYERLIST 1 Spieler
	infoPrint("Anzahl Spieler in der Playerlist: %i",spielerzahl);

	// Playerlist leeren
	preparation_clearPlayers();

	for (int i = 1; i <= MAX_PLAYERS; i++) {
		game_setPlayerName(i, "");
		game_setPlayerScore(i, 0);
	}

	if (game_is_running && spielerzahl < 2) {
		guiShowMessageDialog("Zu wenig Spieler! Spiel wird beendet!", 1); //1=gui main geht nach Bestätigen zum Aufrufer zurück
		pthread_exit(0);
	}

	for(int i=0;i< spielerzahl;i++){
		// kopiere Spieler ID in Spielerliste
		userlist[i].id=packet.content.playerlist[i].id;
		// mehr als 4 Spieler?
		if(i > MAX_PLAYERS){
			infoPrint("Maximale Anzahl an Spieler erreicht!");
			exit(1);
		}

		// kopiere Name aus Paket in Spielerliste
		strncpy(userlist[i].name, packet.content.playerlist[i].playername,PLAYER_NAME_LENGTH);

		// Ausgabe der angemeldeten Spieler
		infoPrint("%s ist angemeldet", userlist[i].name);
		preparation_addPlayer(userlist[packet.content.playerlist[i].id].name);

		game_setPlayerName(i + 1, packet.content.playerlist[i].playername);
		game_setPlayerScore(i + 1, ntohl(packet.content.playerlist[i].score));
		if ((clientID == userlist[i].id)) {
			game_highlightPlayer(i + 1);
			infoPrint("clientID %i trifft zu, hebe Spielername hervor", clientID);
		}
	}
}


/*
 * Funktion wertet empfangen Katalog aus und
 * fuegt diesen der GUI hinzu
 */
void receiveCatalogList(PACKET packet) {
    // Antwort auswerten und anzeigen
    if(ntohs(packet.header.length) > 0){
        infoPrint("%s", packet.content.catalogname);
    	char buffer[ntohs(packet.header.length)];
	    strncpy(buffer, packet.content.catalogname,ntohs(packet.header.length));
	    buffer[ntohs(packet.header.length)] = '\0';
	    preparation_addCatalog(buffer);
    }    
}


/*
 * Funktion wertet empfangen Katalog aus und
 * setzt diesen in der GUI auf aktiv
 */
void receiveCatalogChange(PACKET packet){	      
  if (ntohs(packet.header.length) > 0) {
    char buffer[ntohs(packet.header.length)];
    strncpy(buffer, packet.content.catalogname, ntohs(packet.header.length));
    buffer[ntohs(packet.header.length)] = '\0';
    preparation_selectCatalog(buffer);
  }  
}


/*
 * Funktion wertet Fehlernachrichten aus
 */
void receiveErrorMessage (PACKET packet){
	char error_message[MAX_MESSAGE_LENGTH];
	// hole Errornachricht
	strncpy (error_message, packet.content.error.errormessage, ntohs (packet.header.length)-1);
	// Nullterminierung
	error_message[ntohs (packet.header.length)-1]= '\0';
	// zeige Errordialog + gebe Fehler auf Konsole aus
	errorPrint("Fehler: %s\n", packet.content.error.errormessage);
	// beende Client falls fataler Error
	if(packet.content.error.errortype == ERR_SPIELLEITERLEFTGAME){
		guiShowErrorDialog(error_message, 0);
		exit(0);
	}
	else if((packet.content.error.errortype == ERR_TOOFEWPLAERS) && game_is_running){
		guiShowMessageDialog(error_message, 0);
		exit(0);
	}
}


/*
 * Funktion fordert eine neue Frage vom Server an
 */
void questionRequest(int socketDeskriptor){
	PACKET packet;
	packet.header.type = RFC_QUESTIONREQUEST;
	packet.header.length = 0;
	sendPacket(packet, socketDeskriptor);
	infoPrint("Question Request gesendet!");
}


void *listener_main(int * sockD){

    // warte auf Antwort des Servers
    PACKET response = recvPacket(*sockD);

    // RFC_LOGINRESPONSEOK
    if(response.header.type == RFC_LOGINRESPONSEOK){
        infoPrint("Login Response ok");
        clientID = response.content.loginresponseok.clientid;

        infoPrint("Fordere verfuegbare Kataloge an");
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
	while(stop == 0){
		PACKET packet = recvPacket(*sockD);
		if(equalLiteral(packet.header, "CRE")) {

		}

		switch (packet.header.type){
            // RFC_CATALOGRESPONSE
            case RFC_CATALOGRESPONSE:
                receiveCatalogList(packet);
                break;
            // RFC_CATALOGCHANGE
            case RFC_CATALOGCHANGE:
                receiveCatalogChange(packet);
                break;
			// RFC_PLAYERLIST
			case RFC_PLAYERLIST:
				receivePlayerlist(packet);
				break;
			// RFC_STARTGAME
			case RFC_STARTGAME:
				infoPrint("Spiel gestartet!");
				game_is_running = true;
				questionRequest(*sockD);

				// Vorbereitungsfenster ausblenden und Spielfenster anzeigen
				game_showWindow();
				preparation_hideWindow();
				break;

			case RFC_QUESTION:
				infoPrint("Frage erhalten");

				char msg[50];
				if (packet.header.length != 0) {
					game_clearAnswerMarksAndHighlights();
					game_setQuestion(packet.content.question.question);
					for (int i = 0; i <= 3; i++) {
						game_setAnswer(i, packet.content.question.answers[i]);
					}
					game_setControlsEnabled(1);
					sprintf(msg, "Sie haben %i Sekunden Zeit\n",
							packet.content.question.timeout);
					game_setStatusIcon(0);
					// game_setStatusText(msg);
				} else if (ntohs(packet.header.length) == 0) {
					sprintf(msg, "Alle Fragen beantwortet, bitte Warten.\n");
					game_setStatusText(msg);
					game_setStatusIcon(0);
				}

				game_setStatusText(msg);
				break;
			case RFC_QUESTIONRESULT:
				infoPrint("Frageauswertung erhalten!");

				game_setControlsEnabled(0);

				if (ntohs(packet.header.length) > 0) {
					infoPrint("Korrekte Antwort: %i", packet.content.questionresult.correct);
					infoPrint("Spieler Antowrt: %i", packet.content.questionresult.timeout);
					for (int i = 0; i < NUM_ANSWERS; i++) {
						if (packet.content.questionresult.correct & (1 << i)) {
							game_markAnswerCorrect(i);
						} else {
							game_markAnswerWrong(i);
						}
					}

					if (packet.content.questionresult.timeout != 0) {
						game_setStatusText("Zeit vorbei");
						game_setStatusIcon(3);
					} else if (selection != packet.content.questionresult.correct) {
						game_setStatusText("Falsch");
						game_setStatusIcon(2);
					} else {
						game_setStatusText("Richtige Antwort!");
						game_setStatusIcon(1);
					}
					sem_post(&frage); //fragesemaphor aufrufen
				}

				break;
			case RFC_GAMEOVER:
				sprintf(msg, "Glückwunsch, Sie wurden %i.\n", packet.content.playerrank);
				guiShowMessageDialog(msg, 1); //1=gui main geht nach Bestätigen zum Aufrufer zurück

				pthread_exit(0);
				return NULL;
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
