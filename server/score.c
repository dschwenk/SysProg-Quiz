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
		/*
		 * http://linux.die.net/man/3/sem_wait
		 *
		 * decrements (locks) the semaphore pointed to by sem. If the semaphore's value is greater
		 * than zero, then the decrement proceeds, and the function returns, immediately. If the
		 * semaphore currently has the value zero, then the call blocks until either it becomes
		 * possible to perform the decrement (i.e., the semaphore value rises above zero), or a
		 * signal handler interrupts the call.
		 */
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
