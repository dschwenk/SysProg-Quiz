/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * score.h: Header für den Score Agent
 */

#ifndef SCORE_H
#define SCORE_H

#include <semaphore.h>
#include <stdbool.h>
#include <stdlib.h>

#include "common/rfc.h"
#include "user.h"


sem_t semaphor_score;
void* score_main();

#endif
