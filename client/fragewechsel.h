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
#include <semaphore.h>

sem_t frage;

void *fragewechsel_main(int*);

#endif
