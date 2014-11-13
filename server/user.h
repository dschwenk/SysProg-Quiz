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

#include <stdint.h>

// Struct fuer Spielerverwaltung - Name, ID, Socket, Score
typedef struct player {
	int id;
	char name[32];
	int sockDesc;
	uint32_t score;
	int GameOver;
} PLAYER;


void initSpielerverwaltung();
int addPlayer(char*,int,int);
void sendToAll(PACKET);
void sendPlayerList();
int create_user_mutex();
void lock_user_mutex();
void unlock_user_mutex();
int countUser();
void setRank();

#endif
