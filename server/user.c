/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * user.c: Implementierung der User-Verwaltung
 */

#include "user.h"
#include "common/rfc.h"
#include "common/util.h"
#include "common/networking.h"
#include "login.h"
#include "catalog.h"
#include "main.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>


// Array fuer die Spielerverwaltung
USER spieler[MAX_PLAYERS];

// Mutex fuer die Spielerverwaltung
pthread_mutex_t user_mutex;


/*
 * Funktion initialisert die Spielerverwaltung
 * (Alle Spieler werden mit default-Werten initialisiert
 */
void initSpielerverwaltung(){
	debugPrint("Initialisiere Spielervewaltung.");
	// initialisiere Spieler mit Standardwerten
	for(int i = 0; i < MAX_PLAYERS; i++){
		spieler[i].id = -1;
		spieler[i].name[0] = '\0';
		spieler[i].sockDesc = 0;
		spieler[i].score = 0;
		spieler[i].GameOver = 0;
	}
}


/*
 * Fuegt Spieler zur UserListe hinzu
 *
 * char* name Name des Spielers
 * int length Laenge des Namen
 * int sock Socket des Clients mit diesem Namen
 *
 * return ID des Users, falscher Name(-1) oder max. Anzahl an Spieler erreicht (4)
 */
int addPlayer(char *name, int length, int sock){
	debugPrint("Fuege Spieler zur Verwaltung hinzu.");
	name[length] = '\0';
	int current_count_user = countUser();
	// sind noch freie Spielerplaetze vorhanden
	if(current_count_user >= MAX_PLAYERS){
		return MAX_PLAYERS;
	}
	else {
		// pruefe auf gleichen Namen
		for(int i = 0; i < current_count_user; i++){
			if(strncmp(spieler[i].name, name, length+1) == 0){
				return -1;
			}
		}
		// fuege Spieler zur Verwaltung hinzu
		int new_id = current_count_user;
		spieler[new_id].id = new_id;
		strncpy(spieler[new_id].name, name, length+1);
		spieler[new_id].sockDesc = sock;
		// gebe Spieler-ID zurueck
		return new_id;
	}
}


/*
 * Entfernt Spieler aus der UserListe
 *
 * int client_id ID des Spielers der entfernt wird
 *
 */
int removePlayer(int client_id){
	debugPrint("Loesche Spieler - setzte Daten auf Standard zurueck!");
	int i = 0;
	int current_user_count = countUser();
	// suche Spieler im Array
	while(spieler[i].id != client_id){
		i++;
	}
	// setze Werte auf Standardwerte zurueck
	spieler[i].id = -1;
	spieler[i].name[0] = '\0';
	spieler[i].sockDesc = 0;
	spieler[i].score = 0;
	spieler[i].GameOver = 0;
	// gehe Spielerliste durch und setze geloeschten / default Spieler an letzte Stelle von Array
	while(i < current_user_count){
		USER temp = spieler[i];
		spieler[i] = spieler[i+1];
		spieler[i+1] = temp;
		i++;
	}
	return 0;
}


/*
 * Sende Nachricht an alle Clients
 *
 * PACKET packet Paket das an alle Clients versendet wird
 *
 */
void sendToAll(PACKET packet) {
	debugPrint("Sende Paket an alle Spieler.");
	// gehe alle Spieler durch und Sende Nachricht
	for(int i = 0; i < countUser(); i++){
		sendPacket(packet, spieler[i].sockDesc);
	}
	return;
}


/*
 * Funktion sendet die aktuelle Spielerliste mit
 * Punktzahl an alle Spieler
 */
void sendPlayerList(){
	debugPrint("Spielerliste senden.");

	PACKET packet;
	packet.header.type[0] = 'L';
	packet.header.type[1] = 'S';
	packet.header.type[2] = 'T';

	int user_count = countUser();
	// aktualisiere Spielstand / Rangfolge
	setPlayerRanking();

	for(int i = 0; i < user_count; i++){
		// fuege Spieler zur Liste hinzu
		PLAYERLIST playerlist;
		playerlist.id = spieler[i].id;
		strncpy(playerlist.playername, spieler[i].name, PLAYER_NAME_LENGTH);
		playerlist.score = htonl(spieler[i].score);
		packet.content.playerlist[i] = playerlist;
	}
	// Laenge der Message: Anzahl der Spieler * Groeße der PLAYERLIST
	packet.header.length = htons(sizeof(PLAYERLIST) * user_count);
	// PlayerList an alle Clients senden
	sendToAll(packet);
}


/*
 * Funktion zaehlt aktuell angemeldete Spieler
 */
int countUser(){
	debugPrint("Zaehle aktuell angemeldete Spieler.");
	int current_user_count = 0;
	for(int i=0;i<MAX_PLAYERS;i++){
		// Spieler vorhanden - erhoehe Zaehler
		if((spieler[i].id != -1) && (spieler[i].sockDesc != 0)){
			current_user_count++;
		}
	}
	// gebe Anzahl an Spielern zurueck
	return current_user_count;
}


/*
 * Funktion sendet den aktuellen Katalog an alle Spieler (ausser an Spielleiter)
 */
void sendCatalogChange(){
	debugPrint("Sende aktuellen Katalog an alle Spieler (ausser an Spielleiter)");
	PACKET packet;
	// hole aktuellen Katalog
	packet = getActiveCatalog();
	// sende gewaehlten Katalog an alle Spieler (ausser an Spielleiter)
	for(int i = 0; i < countUser(); i++){
		if(spieler[i].id != 0){
			sendPacket(packet, spieler[i].sockDesc);
		}
	}
	return;
}


/*
 * Funktion sortiert die Spieler nach ihrer Punktzahl
 */
void setPlayerRanking(){
	int current_user_count = countUser();
	debugPrint("Sortiere Spieler nach Punktzahl.");
	// gehe Spieler durch
	for(int i=current_user_count;i>= 0;i--){
		for(int n=0;n<(current_user_count - 1);n++){
			// vergleiche Spielstaende - ist Spielstand des nachfolgender groesser - tausche Plaetze
			if(spieler[n].score < spieler[n+1].score){
				USER temp = spieler[n];
				spieler[n] = spieler[n+1];
				spieler[n+1] = temp;
			}
		}
	}
}


/*
 * gebe Spieler zurueck
 * param int client_id ID des Spielers
 */
USER getUser(int client_id){
	// suche Spieler im Spielerverwaltungsarray
	for(int i = 0; i < MAX_PLAYERS; i++){
		if(spieler[i].id == client_id){
			return spieler[i];
		}
	}
}


/*
 * Pruefe ob alle Spieler alle Fragen beantwortet haben
 */
int isGameOver(){
	for(int i=0;i<countUser();i++){
		// haben alle Spieler ihre Fragen beantwortet
		if(spieler[i].GameOver == 0){
			return 0;
		}
	}
	return 1;
}


/*
 * Funktion aktualisiert den Spielstatus eines Spielers
 * wenn dieser alle Fragen beantwortet hat
 */
void sendGameOver(int id){
	int i = 0;
	// suche Spieler der das Spiel beendet hat
	while(spieler[i].id != id){
		i++;
	}
	spieler[i].GameOver = 1;
	if(isGameOver() == 1){
		// sende an Spieler EndPlatzierung
		for(int j=0;j<countUser();j++){
			PACKET packet;
			packet.header.type[0] = 'G';
			packet.header.type[1] = 'O';
			packet.header.type[2] = 'V';
			packet.header.length = htons(1);
			packet.content.playerrank = j + 1;
			sendPacket(packet, spieler[j].sockDesc);
		}
		// Server beenden
		endServer();
		exit(0);
		return;
	}
	return;
}


/*
 * Funktion aktualisiert den Punktestand eines Spielers
 */
void setUserScore(int player_id, int score){
	for(int i=0;i< countUser();i++){
		if(spieler[i].id == player_id){
			spieler[i].score += score;
		}
	}
	return;
}


/*
 * Funktion initalisiert Mutex fuer die Benutzerdaten
 */
int create_user_mutex(){
	// initialisiere Mutex, NULL -> Standardwerte
	// http://www.lehman.cuny.edu/cgi-bin/man-cgi?pthread_mutex_init+3
	debugPrint("Initialisiere Benutzermutex.");
	if(pthread_mutex_init(&user_mutex, NULL) != 0){
		errorPrint("Fehler beim initialisieren des Benutzermutex.");
		return -1;
	}
	return 0;
}


/*
 * Funktion sperrt Mutex fuer die Benutzerdaten
 */
void lock_user_mutex(){
	debugPrint("lock Benutzerdatenmutex.");
	// lock mutex
	// http://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_mutex_lock.html
	pthread_mutex_lock(&user_mutex);
}


/*
 * Funktion gibt Mutex fuer die Benutzerdaten frei
 */
void unlock_user_mutex(){
	debugPrint("unlock Benutzerdatenmutex.");
	// unlock mutex
	// http://linux.die.net/man/3/pthread_mutex_unlock
	pthread_mutex_unlock(&user_mutex);
}
