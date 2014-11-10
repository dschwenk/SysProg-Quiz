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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>


PLAYER spieler[4];

/*
 * Initialisiert das Array der User
 */

void initSpielerverwaltung(){
	// initialisiere Spieler mit ID -1 und Name '0'
	for(int i = 0; i < 4; i++){
		spieler[i].id = -1;
		spieler[i].name[0] = '0';
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
	for(int i = 0; i < 4; i++){
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
	for(int i = 0; i < 4; i++){
		// ist noch nicht belegt
		if(current_ids[i] == 0){
			for(int c = 0; c < 4; c++){
				if(current_ids[c] == 0){
					// speicher ID, Name + Socket in Spieler
					spieler[c].id = i;
					strncpy(spieler[c].name, name, length + 1);
					spieler[c].sockDesc = sock;
					return i;
				}
			}
		}
	}
	return 4;
}


/*
 * sende PlayerList an alle Spieler
 */

void sendPlayerList(){

	PACKET player;
	player.header.type = RFC_PLAYERLIST;

	int user_count = 0;

	// Spielerliste in PLAYERLIST playerlist schreiben
	for (int i = 0; i < 4; i++) {
		// fuege Spieler zur Liste hinzu
		if(spieler[i].id != -1 && spieler[i].sockDesc != 0){
			PLAYERLIST playerlist;
			playerlist.id = spieler[i].id;
			strncpy(playerlist.playername, spieler[i].name, 32);
			playerlist.score = 0;
			player.content.playerlist[i] = playerlist;
			user_count++;
		}
	}

	// Laenge der Message: Anzahl der Spieler * 37 (Groeße der PLAYERLIST)
	player.header.length = htons(sizeof(PLAYERLIST) * user_count);
	// An alle Clients senden
	for (int c = 0; c < user_count; c++) {
		if (spieler[c].sockDesc != 0) {
			// Packet senden
			sendPacket(player, spieler[c].sockDesc);
		}
	}
}
