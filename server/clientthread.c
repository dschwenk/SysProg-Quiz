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




PACKET errpacket;

void setErrorPaket(PACKET paket){
	errpacket = paket;
}




void *client_thread_main(int* client_id){

	// Variablen fuer Spielablauf
	Question* shmQ;
	int question_number = 0; // Fragenummer
	int correct; // Bitmaske für richtige Antworten
	int time_left = 0; // Zeit fuer Beantwortung der Frage
	uint8_t selection = 0; // vom Spieler gewaehlte Antowrt
	bool time_error = false; // Flag ob ein Fehler bei der Restzeitberechnung auftrat

	// hole Spielerinformationen
	USER spieler;
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

		// Fehlerbehandlung falls bei letzter Frage ein Fehler empfangen wurde
		if(time_error){
			time_error = false;
			packet = errpacket;
		}
		// empfange
		else {
			packet = recvPacket(spieler.sockDesc);
		}

        infoPrint("clientthread received packet: %c%c%c", packet.header.type[0], packet.header.type[1], packet.header.type[2]);

		// werte empfangenes Paket aus

		// ERR - Fehlernachricht empfangen
		if(isStringEqual(packet.header, "ERR")){
			// pruefe Subtyp

			// Spieler hat das Spiel verlassen
			if(packet.content.error.subtype == ERR_WARNING){
				debugPrint("Spieler %s (ID: %d) hat das Spiel verlassen", spieler.name, spieler.id);
				// pruefe ob Spielleiter
				if(is_spielleiter){
					debugPrint("Spieler %s (ID: %d) war Spielleiter", spieler.name, spieler.id);
					infoPrint("Der Spieleiter hat das Spiel verlassen, der Server wird beendet");

					// setzte Fehlertyp + Text
					response.header.type[0] = 'E';
					response.header.type[1] = 'R';
					response.header.type[2] = 'R';
					char *errormsg = "Der Spieleiter hat das Spiel verlassen, der Server wird beendet!";
					size_t length = strlen(errormsg);
					response.header.length = htons(length+1);
					response.content.error.subtype = ERR_SPIELLEITERLEFTGAME;
					strncpy(response.content.error.message, errormsg , length);
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
					// sende aktualisierte Spielerliste an alle verbliebene Spieler
					sem_post(&semaphor_score);
					// pruefe ob Spiel bereits laeft und Anzahl verbliebener Spieler
					if((getGameRunning()) && (countUser() < 2)){
						// zu wenig Spieler
						infoPrint("Zu wenige Spieler, der Server wird beendet");
						response.header.type[0] = 'E';
						response.header.type[1] = 'R';
						response.header.type[2] = 'R';
						char *errormsg = "Zu wenig Spieler, breche Spiel ab.";
						size_t length = strlen(errormsg);
						response.header.length = htons(length+1);
						response.content.error.subtype = ERR_TOOFEWPLAERS_GAME;
						strncpy(response.content.error.message, errormsg , length);
						// sende Fehlermeldung an alle
						lock_user_mutex();
						sendToAll(response);
						unlock_user_mutex();
						// Server beenden
						endServer();
					}
				}
			}
			// ERR_FATAL
			else {
				debugPrint("Fataler Fehler - entferne Spieler!");
				lock_user_mutex();
				removePlayer(spieler.id);
				unlock_user_mutex();
				// sende aktualisierte Spielerliste an alle verbliebene Spieler
				sem_post(&semaphor_score);
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
				response.header.type[0] = 'E';
				response.header.type[1] = 'R';
				response.header.type[2] = 'R';
				char *errormsg = "Zu wenige Spieler um das Spiel zu starten!";
				size_t length = strlen(errormsg);
				response.header.length = htons(length+1);
				response.content.error.subtype = ERR_TOOFEWPLAERS_PREP;
				strncpy(response.content.error.message, errormsg , length);
				sendPacket(response, spieler.sockDesc);
			}
			// genug Spieler - Spiel wird gestartet
			else {
				setGameRunning();
				char catalog[MAX_CATALOG_NAME_LENGTH];

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

				// kopiere Frage + Antworten + Timeout
				QuestionMessage quest_message;
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
					PACKET questionresult;
					// QuestionResult - Auswertung einer Antwort auf eine Quiz-Frage
					questionresult.header.type[0] = 'Q';
					questionresult.header.type[1] = 'R';
					questionresult.header.type[2] = 'E';
					questionresult.header.length = htons(2);
					questionresult.content.questionresult.correct = correct;
					questionresult.content.questionresult.timeout = 1;
					sendPacket(questionresult, spieler.sockDesc);
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
					PACKET questionresult;
					// QuestionResult - Auswertung einer Antwort auf eine Quiz-Frage
					questionresult.header.type[0] = 'Q';
					questionresult.header.type[1] = 'R';
					questionresult.header.type[2] = 'E';
					questionresult.header.length = htons(2);
					questionresult.content.questionresult.correct = correct;
					questionresult.content.questionresult.timeout = 0;
					sendPacket(questionresult, spieler.sockDesc);
					infoPrint("Antwort auf Frage gesendet!");
				}
				// Restzeit = 0
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

	// Filedeskriptor-Menge
	fd_set fds;

	PACKET packet;
	int select = 0;

	struct timespec timeStart, timeEnd, timeRest;

	bool waiting_for_answer = true;

	// aktuelle Systemzeit holen - "Startzeit"
	// CLOCK_MONOTONIC: Eine Uhr, die nicht verstellt werden kann und die Zeit seit einem nicht näher spezifizierten Zeitpunkt (Booten des Systems?) anzeigt.
	clock_gettime(CLOCK_MONOTONIC, &timeStart);

	// Setze Endzeit auf Startzeit + Timeout
	timeEnd.tv_nsec = timeStart.tv_nsec;
	timeEnd.tv_sec = timeStart.tv_sec + timeout;

	// setze Timeout - solange wartet pselect
	timeRest.tv_nsec = 0;
	timeRest.tv_sec = timeout;


	while(waiting_for_answer){

		// Neuinitalisierung von fds, kann durch vorheriges pselect veraendert worden sein
		FD_ZERO(&fds);
		// Filedeskriptor in Set einfuegen
		FD_SET(sockD, &fds);

		// warte bis Filedeskriptoren bereit ist zum lesen (eine Anwort empfangen werden kann) oder die Zeit abgelaufen ist
		select = pselect(sockD + 1, &fds, NULL, NULL, &timeRest, NULL);

		if(select > 0){
			// empfange Nachricht
			packet = recvPacket(sockD);
			// QAN - Quiz-Frage wurde beantwortet
			if(isStringEqual(packet.header, "QAN")) {
				memcpy(selection, &packet.content.selection, 1);
			}
			// es wurde ein Fehler / anderes Paket empfangen
			else {
				errpacket = packet;
				return 0;
			}
			waiting_for_answer = false;
		}

		// aktuelle Systemzeit holen -> "Endzeit"
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

	// Restzeit steht in timeRest
	uint32_t timeleft = 0;
	/*
	 * http://stackoverflow.com/questions/15287778/clock-gettime-returning-a-nonsense-value
	 * Note that the delta is in seconds, which is calculated by dividing the nanoseconds by 1000000, which
	 * combined with subtracting a time from the future from a time from the past which
	 * equals a negative, and then dividing that by 1000, it makes the delta a positive.
	 */
	timeleft = timeRest.tv_nsec / 1000000;
	timeleft = timeleft + (timeRest.tv_sec * 1000);

	return timeleft;
}
