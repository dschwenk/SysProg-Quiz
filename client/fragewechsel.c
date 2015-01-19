/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Client
 * 
 * fragewechsel.c: Implementierung des Fragewechsel-Threads
 */

#include "fragewechsel.h"
#include "listener.h"
#include "common/util.h"

#include <unistd.h>

/*
 * Thread fordert Fragen vom Server an
 */
void *fragewechsel_main(int *sockD) {
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
        // warte auf Freigabe
        sem_wait(&frage);
        sleep(3);
        // Frage vom Server anfordern
        questionRequest(*sockD);
    }
}
