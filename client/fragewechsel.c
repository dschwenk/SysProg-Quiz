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
        // warte auf Freigabe
        sem_wait(&frage);
        sleep(3);
        // Frage vom Server anfordern
        questionRequest(*sockD);
    }
}
