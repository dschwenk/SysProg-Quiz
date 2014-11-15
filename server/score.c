/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * score.h: Implementierung des Score-Agents
 */


#include "score.h"
#include "common/rfc.h"
#include "user.h"


void* score_main(){
	while(1){
		// http://linux.die.net/man/3/sem_wait
		// Spielstand Semaphore sperren
		sem_wait(&semaphor_score);
		// User Mutex sperren
		lock_user_mutex();
		// sende Spielerliste an alle Spieler
		sendPlayerList();
		// Mutex freigeben
		unlock_user_mutex();
	}
	return NULL;
}
