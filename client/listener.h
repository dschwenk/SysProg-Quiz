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
	char name[32];
} USER;


void receivePlayerlist(PACKET);
void receiveErrorMessage(PACKET);
void game_onAnswerClicked(int, int);
void *listener_main();

#endif

