/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * user.h: Header für die User-Verwaltung
 */

#ifndef USER_H
#define USER_H


// Struct fuer Spielerverwaltung - Name, ID, Socket
typedef struct player {
	int id;
	char name[32];
	int sockDesc;
} PLAYER;


void initSpielerverwaltung();
int addPlayer(char*,int,int);
void sendPlayerList();

#endif
