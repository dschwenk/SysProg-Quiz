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
PLAYER spieler[MAX_PLAYERS];

// Mutex fuer die Spielerverwaltung
pthread_mutex_t user_mutex;




// Initialisiere Spielerarray
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
	name[length] = 0;
	int current_count_user = countUser();
	lock_user_mutex();
	// sind noch freie Spielerplaetze vorhanden
	if(current_count_user >= MAX_PLAYERS){
		unlock_user_mutex();
		return MAX_PLAYERS;
	}
	else {
		// pruefe auf gleichen Namen
		for(int i = 0; i < current_count_user; i++){
			if(strncmp(spieler[i].name, name, length + 1) == 0){
				unlock_user_mutex();
				return -1;
			}
		}
		// fuege Spieler zur Verwaltung hinzu
		int new_id = current_count_user;
		spieler[new_id].id = new_id;
		strncpy(spieler[new_id].name, name, length + 1);
		spieler[new_id].sockDesc = sock;
		// gebe Spieler-ID zurueck
		unlock_user_mutex();
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
	lock_user_mutex();
	// suche Spieler im Array
	while(spieler[i].id != client_id){
		i++;
	}
	// setze Werte auf Standardwerte zurueck
	spieler[i].id = -1;
	spieler[i].name[0] = '\0';
	spieler[i].sockDesc = 0;
	spieler[i].score = 0;
	// gehe Spielerliste durch und setze geloeschten / default Spieler an letzte Stelle von Array
	while(i < current_user_count){
		PLAYER temp = spieler[i];
		spieler[i] = spieler[i+1];
		spieler[i+1] = temp;
		i++;
	}
	unlock_user_mutex();
	// aktualisiere Spielstand / Rangfolge
	setPlayerRanking();
	// sende PlayerList an alle Spieler
	sendPlayerList();
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
	int current_count_players = countUser();
	// gehe alle Spieler durch und Sende Nachricht
	for(int i = 0; i < current_count_players; i++){
		sendPacket(packet, spieler[i].sockDesc);
	}
	return;
}



// sende PlayerList an alle Spieler
void sendPlayerList(){
	debugPrint("Spielerliste senden.");

	PACKET packet;
	packet.header.type = RFC_PLAYERLIST;

	int user_count = countUser();

	for(int i = 0; i < user_count; i++){
		// fuege Spieler zur Liste hinzu
		PLAYERLIST playerlist;
		playerlist.id = spieler[i].id;
		strncpy(playerlist.playername, spieler[i].name, PLAYER_NAME_LENGTH);
		playerlist.score = 0;
		packet.content.playerlist[i] = playerlist;
	}

	// Laenge der Message: Anzahl der Spieler * 37 (Groeße der PLAYERLIST)
	packet.header.length = htons(sizeof(PLAYERLIST) * user_count);
	// PlayerList an alle Clients senden
	sendToAll(packet);
}



// zaehlt aktuelle Anzahl an Spielern
int countUser(){
	lock_user_mutex();
	debugPrint("Zaehle aktuell angemeldete Spieler.");
	int current_user_count = 0;
	for(int i = 0; i < MAX_PLAYERS; i++){
		// Spieler vorhanden - erhoehe Zaehler
		if((spieler[i].id != -1) && (spieler[i].sockDesc != 0)){
			current_user_count++;
		}
	}
	unlock_user_mutex();
	// gebe Anzahl an Spielern zurueck
	return current_user_count;
}



// sende aktuellen Katalog an alle Spieler
void sendCatalogChange(){
	debugPrint("Sende aktuellen Katalog an alle Spieler.");
	PACKET packet;
	// hole aktuellen Katalog
	packet = getActiveCatalog();
	// sende Nachricht mit aktuellem Katalog an alle
	sendToAll(packet);
	return;
}



// sortiere Spieler nach Punkten
void setPlayerRanking(){
	int current_user_count = countUser();
	debugPrint("Sortiere Spieler nach Punktzahl.");
	// gehe Spieler durch
	for(int i = current_user_count; i >= 0; i--){
		for(int n = 0; n < (current_user_count - 1); n++){
			// vergleiche Spielstaende - ist Spielstand des nachfolgender groesser - tausche Plaetze
			if(spieler[n].score < spieler[n+1].score){
				PLAYER temp = spieler[n];
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
PLAYER getUser(int client_id){
	for(int i = 0; i < MAX_PLAYERS; i++){
		if(spieler[i].id == client_id){
			return spieler[i];
		}
	}
}


// pruefe ob Spieler alle Fragen beantwortet
int isGameOver(){
	for(int i=0;i<countUser();i++){
		// haben alle Spieler ihre Fragen beantwortet
		if(spieler[i].GameOver == 0){
			return 0;
		}
	}
	return 1;
}


//
void sendGameOver(int id){
	int i = 0;
	while(spieler[i].id != id){
		i++;
	}
	if(isGameOver() == 1){
		// sende an Spieler EndPlatzierung
		for(int j=0;j<countUser();j++){
			PACKET packet;
			packet.header.type = RFC_GAMEOVER;
			packet.header.length = htons(1);
			packet.content.playerrank = j + 1;
			sendPacket(packet, spieler[j].sockDesc);
		}
		// Server + Thread beenden
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



// Mutex fuer die Benutzerdaten initalisieren
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



// Zugriff auf Benutzerdaten sperren
void lock_user_mutex(){
	debugPrint("lock Benutzerdatenmutex.");
	// lock mutex
	// http://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_mutex_lock.html
	pthread_mutex_lock(&user_mutex);
}


// Zugriff auf Benutzerdaten erlauben
void unlock_user_mutex(){
	debugPrint("unlock Benutzerdatenmutex.");
	// unlock mutex
	// http://linux.die.net/man/3/pthread_mutex_unlock
	pthread_mutex_unlock(&user_mutex);
}
