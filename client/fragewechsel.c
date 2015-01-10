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


void *fragewechsel_main(int *sockD) {
    while(1){
        infoPrint("While läuft!");
        sem_wait(&frage);
        infoPrint("Sem läuft!");
        sleep(3);
        // Frage vom Server anfordern
        questionRequest(*sockD);
    }
}
