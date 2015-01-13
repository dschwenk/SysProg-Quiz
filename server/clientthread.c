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
#include "clientthread.h"
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
#include <time.h>
#include <stdio.h>


void *client_thread_main(int* client_id){

	// Variablen fuer Spielablauf
	Question* shmQ;
	int question_number = 0; // Fragenummer
	int correct; // Bitmaske für richtige Antworten
	int time_left = 0; // Zeit fuer Beantwortung der Frage
	uint8_t selection = 5; // vom Spieler gewaehlte Antowrt
	bool time_error = false; // Flag ob ein Fehler bei der Restzeitberechnung auftrat

	// hole Spielerinformationen
	PLAYER spieler;
	lock_user_mutex();
	spieler = getUser(*client_id);
	unlock_user_mutex();

	// ist der Spieler Spielleiter
	bool is_spielleiter = false;
	if(spieler.id == 0){
		is_spielleiter = true;
	}

	// Struktur zum empfangen + senden von Nachrichten
	PACKET packet, response;

	// Empfangsschleife
	while(1){
		// Fehlerbehandlung falls bei letzter Frage ein Zeitfehler auftrat
		if(time_error){
			packet.header.type[0] = 'E';
			packet.header.type[1] = 'R';
			packet.header.type[2] = 'R';
			time_error = false;
		}
		// empfange
		else {
			packet = recvPacket(spieler.sockDesc);
		}
        infoPrint("clientthread packet: %d", packet.header.type);

		// werte empfangenes Paket aus

		// ERR - Fehlernachricht empfangen
		if(isStringEqual(packet.header, "ERR")){
			// pruefe Subtyp
			// Spieler hat das Spiel verlassen
			if(packet.content.error.errortype == ERR_CLIENTLEFTGAME){
				debugPrint("Spieler %s (ID: %d) hat das Spiel verlassen", spieler.name, spieler.id);
				// pruefe ob Spielleiter
				if(is_spielleiter){
					debugPrint("Spieler %s (ID: %d) war Spielleiter", spieler.name, spieler.id);
					infoPrint("Der Spieleiter hat das Spiel verlassen, der Server wird beendet");
					// setzte Fehlertyp + Text
					packet.header.type[0] = 'E';
					packet.header.type[1] = 'R';
					packet.header.type[2] = 'R';
					response.header.length = htons(strlen("Der Spieleiter hat das Spiel verlassen, der Server wird beendet!"));
					response.content.error.errortype = ERR_SPIELLEITERLEFTGAME;
					strncpy(response.content.error.errormessage,"Der Spieleiter hat das Spiel verlassen, der Server wird beendet!",	ntohs(response.header.length));
					// sende Fehlermeldung an alle
					lock_user_mutex();
					sendToAll(response);
					unlock_user_mutex();
					// Server beenden
					endServer();
				}
				// Spieler war kein Spieleiter
				else {
					debugPrint("Spieler %s (ID: %d) war NICHT Spielleiter", spieler.name, spieler.id);
					// entferne Spieler aus verwaltung
					lock_user_mutex();
					removePlayer(spieler.id);
					unlock_user_mutex();
					// pruefe ob Spiel bereits laeft und Anzahl verbliebener Spieler
					if((getGameRunning()) && (countUser() <= 2)){
						// zu wenig Spieler
						packet.header.type[0] = 'E';
						packet.header.type[1] = 'R';
						packet.header.type[2] = 'R';
						response.header.length = htons(strlen("Zu wenig Spieler, breche Spiel ab."));
						response.content.error.errortype = ERR_TOOFEWPLAERS;
						strncpy(response.content.error.errormessage,"Zu wenig Spieler, breche Spiel ab.",ntohs(response.header.length));
						// sende Fehlermeldung an alle
						lock_user_mutex();
						sendToAll(response);
						unlock_user_mutex();
						// Server beenden
						endServer();
					}
					// sende aktualisierte Spielerliste an alle verbliebene Spieler
					// sendPlayerList();
					sem_post(&semaphor_score);
				}
			}
			else {
				lock_user_mutex();
				removePlayer(spieler.id);
				unlock_user_mutex();
				if(getGameRunning()){
					sendGameOver(0);
				}
			}
			// beende Thread
			pthread_exit(0);
			return NULL;
		}

		// CRQ - Anforderung der Liste der Fragekataloge
		else if(isStringEqual(packet.header, "CRQ")){
			debugPrint("Catalog Request von Spieler-ID: %i Name: %s", spieler.id, spieler.name);
			lock_user_mutex();
			sendCatalog(spieler.sockDesc);
			unlock_user_mutex();
			// pruefe ob Katalog ausgewaehlt wurde - falls ja sende Katalog
			if(isCatalogChosen()){
				// Client erkennt somit den vom Spielleiter ausgewaehlten Katalog
				sendCatalogChange();
			}
		}

		// CCH - Spielleiter hat Katalogauswahl geaendert
		else if(isStringEqual(packet.header, "CCH")){
			debugPrint("Catalog Change von Spieler-ID: %i Name: %s", spieler.id, spieler.name);
			// setzte aktiven Katalog
			lock_user_mutex();
			setActiveCatalog(packet);
			unlock_user_mutex();
			// sende Katalogwechsel an alle Spieler
			sendCatalogChange();
		}

		// STG - Spielleiter moechte Spiel starten
		else if(isStringEqual(packet.header, "STG")){
			// pruefe Anzahl angemelder Spieler - >= 2 Spieler zum Spielstart benoetigt
			// zu wenig Spieler - Spiel wird nicht gestartet
			if(countUser() < 2){
				infoPrint("Zu wenige Spieler um das Spiel zu starten!");
				packet.header.type[0] = 'E';
				packet.header.type[1] = 'R';
				packet.header.type[2] = 'R';
				response.header.length = htons(strlen("Zu wenige Spieler um das Spiel zu starten!"));
				response.content.error.errortype = ERR_TOOFEWPLAERS;
				strncpy(response.content.error.errormessage,"Zu wenige Spieler um das Spiel zu starten!", ntohs(response.header.length));
				sendPacket(response, spieler.sockDesc);
			}
			// genug Spieler - Spiel wird gestartet
			else {
				setGameRunning();
				char catalog[CATALOG_NAME_LENGTH];

				// lade Fragen des aktiven Katalogs
				strncpy(catalog, packet.content.catalogname,ntohs(packet.header.length));
				catalog[ntohs(packet.header.length)] = '\0';
				loadQuestions(catalog);

				// sende Paket mit aktuellem Katalog an alle Spieler
				lock_user_mutex();
				sendToAll(packet);
				unlock_user_mutex();
			}
		}

		// QRQ - Anforderung einer Quizfrage
		else if(isStringEqual(packet.header, "QRQ")){
			// lade naechste Frage
			shmQ = getQuestion(question_number);
			question_number++;
			PACKET question_packet;
			// gibt es noch eine Frage
			if(strcmp(shmQ->question, "") != 0){
				// Bitmaske für richtige Antworten
				correct = shmQ->correct;

				question_packet.header.type[0] = 'Q';
				question_packet.header.type[1] = 'U';
				question_packet.header.type[2] = 'E';
				question_packet.header.length = htons(sizeof(QuestionMessage));
				QuestionMessage quest_message;

				// kopiere Frage + Antworten + Timeout
				strcpy(quest_message.question, shmQ->question);
				strcpy(quest_message.answers[0], shmQ->answers[0]);
				strcpy(quest_message.answers[1], shmQ->answers[1]);
				strcpy(quest_message.answers[2], shmQ->answers[2]);
				strcpy(quest_message.answers[3], shmQ->answers[3]);
				quest_message.timeout = shmQ->timeout;

				// versende Fragenpaket + aktualisierte Spielerliste
				question_packet.content.question = quest_message;
				sendPacket(question_packet, spieler.sockDesc);
				lock_user_mutex();
				sendPlayerList();
				unlock_user_mutex();

				// warte auf Clientantwort
				time_left = questionTimer(&selection, shmQ->timeout, spieler.sockDesc);

				// werte Restzeit aus - Zeit abgelaufen
				if(time_left == -1){
					PACKET QuestionAnswer;
					// QuestionResult - Auswertung einer Antwort auf eine Quiz-Frage
					QuestionAnswer.header.type[0] = 'Q';
					QuestionAnswer.header.type[1] = 'R';
					QuestionAnswer.header.type[2] = 'E';
					QuestionAnswer.header.length = htons(2);
					QuestionAnswer.content.questionresult.correct = correct;
					QuestionAnswer.content.questionresult.timeout = 1;
					sendPacket(QuestionAnswer, spieler.sockDesc);
					infoPrint("Antwort auf Frage gesendet!");
					// The sem_post() function unlocks the semaphore referenced by sem by performing a semaphore unlock operation on that semaphore.
					sem_post(&semaphor_score);
				}
				// es ist noch Restzeit vorhanden
				else if(time_left != 0){
					infoPrint("Gewaehlte Antwort: %d", selection);
					// ist die Antwort korrekt
					if(selection == correct){
						infoPrint("Restzeit: %i", time_left);
						// berechne Punktzahl (siehe Aufgabenblatt 6)
						unsigned long score = (time_left * 1000UL) / (shmQ->timeout * 1000UL);
						/* auf 10 er - Stellen runden */
						score = ((score + 5UL) / 10UL) * 10UL;
						infoPrint("Erreichte Punktzahl: %lu", score);
						// Punkte in Benutzerverwaltung speichern
						lock_user_mutex();
						setUserScore(spieler.id, score);
						unlock_user_mutex();
						// The sem_post() function unlocks the semaphore referenced by sem by performing a semaphore unlock operation on that semaphore.
						sem_post(&semaphor_score);
					}
					PACKET QuestionAnswer;
					// QuestionResult - Auswertung einer Antwort auf eine Quiz-Frage
					QuestionAnswer.header.type[0] = 'Q';
					QuestionAnswer.header.type[1] = 'R';
					QuestionAnswer.header.type[2] = 'E';
					QuestionAnswer.header.length = htons(2);
					QuestionAnswer.content.questionresult.correct = correct;
					QuestionAnswer.content.questionresult.timeout = 0;
					sendPacket(QuestionAnswer, spieler.sockDesc);
					infoPrint("Antwort gesendet!");
				}
				// Fehler
				else {
					time_error = true;
				}
			}
			// keine weiteren Fragen
			else {
				// Question - Reaktion auf QuestionRequest, Transport einer Quiz-Frage zum Client
				question_packet.header.type[0] = 'Q';
				question_packet.header.type[1] = 'U';
				question_packet.header.type[2] = 'E';
				question_packet.header.length = htons(0);
				sendPacket(question_packet, spieler.sockDesc);
				sendGameOver(spieler.id);
			}
		}
		// unbekannter Nachrichtentyp
		else {
			debugPrint("Unbekannter Nachrichtentyp: %i%i%i von Spieler-ID: %i Name: %s", packet.header.type[0], packet.header.type[1], packet.header.type[2], spieler.id, spieler.name);
		}
	}
	pthread_exit(0);
	return NULL;
}


/*
 * Funktion wartet auf die Antwort zu einer Frage und
 * gibt die Restzeit zurueck
 */
int questionTimer(uint8_t* selection, int timeout, int sockD){

	fd_set fds;

	PACKET packet;
	int select = 0;

	struct timespec timeStart, timeEnd, timeRest;

	bool waiting_for_answer = true;

	// aktuelle Systemzeit holen
	// CLOCK_MONOTONIC: Eine Uhr, die nicht verstellt werden kann und die Zeit seit einem nicht näher spezifizierten Zeitpunkt (Booten des Systems?) anzeigt.
	clock_gettime(CLOCK_MONOTONIC, &timeStart);

	// Setze Endzeit auf Startzeit + Timeout
	timeEnd.tv_nsec = timeStart.tv_nsec;
	timeEnd.tv_sec = timeStart.tv_sec + timeout;

	timeRest.tv_nsec = 0;
	timeRest.tv_sec = timeout;

	// solange keine Antwort eingetroffen ist laeuft die Zeit ab
	while(waiting_for_answer){

		FD_ZERO(&fds);
		FD_SET(sockD, &fds);

		select = pselect(sockD + 1, &fds, NULL, NULL, &timeRest, NULL);

		if(select > 0){
			// empfange Nachricht
			packet = recvPacket(sockD);
			// QAN - Quiz-Frage wurde beantwortet
			if(isStringEqual(packet.header, "QAN")) {
				memcpy(selection, &packet.content.selection, 1);
			}
			else {
				return 0;
			}
			waiting_for_answer = false;
		}

		// aktuelle Systemzeit holen
		clock_gettime(CLOCK_MONOTONIC, &timeStart);

		// vergleiche Start & Endzeit - pruefe ob Zeit abgelaufen
		if(cmpTimespec(&timeStart, &timeEnd) != -1){
			infoPrint("Zeit abgelaufen!");
			return -1;
		}
		else {
			// subtrahiere Zeitwerte
			timeRest = timespecSub(&timeEnd, &timeStart);
		}
	}

	uint32_t timeleft = 0;
	timeleft = timeRest.tv_nsec / 1000000;
	timeleft = timeleft + (timeRest.tv_sec * 1000);

	return timeleft;
}
