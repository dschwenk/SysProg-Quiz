/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Client
 * 
 * listener.h: Header für den Listener-Thread
 */

#ifndef LISTENER_H
#define LISTENER_H

#include "common/rfc.h"


// Spieler - ID + Name
typedef struct {
	int id;
	char name[PLAYER_NAME_LENGTH];
} USER;


void receivePlayerlist(PACKET);
void receiveCatalogList(PACKET);
void receiveCatalogChange(PACKET);
void receiveErrorMessage(PACKET);
void questionRequest(int);
void *listener_main(int *);

#endif
