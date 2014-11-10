/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Client
 * 
 * fragewechsel.h: Header für den Fragewechsel-Thread
 */

#ifndef FRAGEWECHSEL_H
#define FRAGEWECHSEL_H

#include "../common/rfc.h"

/* ... */
typedef struct {
	int id;
	char name[32];
}USER;
USER userlist[4];

void ErrorMessageEmpfangen(PACKET packet);
void *listener_main();

#endif
