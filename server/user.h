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
#include "common/rfc.h"


// Struct fuer Spielerverwaltung - Name, ID, Socket, Score, Flag fuer Spielende
typedef struct player {
	int id;
	char name[PLAYER_NAME_LENGTH];
	int sockDesc;
	uint32_t score;
	int GameOver;
} PLAYER;


void initSpielerverwaltung();
int addPlayer(char*,int,int);
int removePlayer(int);
void sendToAll(PACKET);
void sendPlayerList();
int countUser();
void sendCatalogChange();
void setPlayerRanking();
PLAYER getUser(int);
int isGameOver();
void sendGameOver(int);
void setUserScore(int,int);
int create_user_mutex();
void lock_user_mutex();
void unlock_user_mutex();




#endif
