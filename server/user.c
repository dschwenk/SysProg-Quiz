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
	name[length] = 0;
	int current_ids[4] = { 0 };
	for(int i = 0; i < MAX_PLAYERS; i++){
		// Auf gleichen Namen pruefen
		if(strncmp(spieler[i].name, name, length + 1) == 0){
			infoPrint("Name bereits vorhanden!");
			return -1;
		}
		// Belegte Plaetze erkennen
		if(spieler[i].id != -1){
			// es gibt einen Spieler mit der aktuellen ID - merke in current_ids
			current_ids[spieler[i].id] = 1;
		}
	}
	// Naechsten leeren Platz suchen
	for(int i = 0; i < MAX_PLAYERS; i++){
		// ist noch nicht belegt
		if(current_ids[i] == 0){
			for(int c = 0; c < 4; c++){
				if(current_ids[c] == 0){
					// speicher ID, Name + Socket in Spieler
					spieler[c].id = i;
					strncpy(spieler[c].name, name, length + 1);
					spieler[c].sockDesc = sock;
					// gebe Spieler-ID zurueck
					return i;
				}
			}
		}
	}
	// kein freier Platz - max. Anzahl an Spielern erreicht
	return 4;
}



/*
 * Entfernt Spieler aus der UserListe
 *
 * int id ID des Spielers der entfernt wird
 *
 */
int removePlayer(int id) {
	int i = 0;
	int current_user_count = countUser();
	// suche Spieler im Array
	while(spieler[i].id != id){
		i++;
	}
	// setze Werte auf Standardwerte zurueck
	debugPrint("Loesche Spieler - setzte Daten auf Standard zurueck!");
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
	// aktualisiere Spielstand / Rangfolge
	setRank();
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
	int current_count_players = countUser();
	// gehe alle Spieler durch und Sende Nachricht
	for(int i = 0; i < current_count_players; i++){
		sendPacket(packet, spieler[i].sockDesc);
	}
	return;
}



// sende PlayerList an alle Spieler
void sendPlayerList(){

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
	int current_user_count = 0;
	for(int i = 0; i < MAX_PLAYERS; i++){
		// Spieler vorhanden - erhoehe Zaehler
		if((spieler[i].id != -1) && (spieler[i].sockDesc != 0)){
			current_user_count++;
		}
	}
	// gebe Anzahl an Spielern zurueck
	return current_user_count;
}



// sende aktuellen Katalog an alle Spieler
void sendCatalogChange(){
	PACKET packet;
	// hole aktuellen Katalog
	packet = getActiveCatalog();
	// sende Nachricht mit aktuellem Katalog an alle
	sendToAll(packet);
	return;
}



// sortiere Spieler nach Punkten
void setRank(){
	int current_user_count = countUser();
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


// Mutex fuer die Benutzerdaten initalisieren
int create_user_mutex(){
	// initialisiere Mutex, NULL -> Standardwerte
	// http://www.lehman.cuny.edu/cgi-bin/man-cgi?pthread_mutex_init+3
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
