/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * clientthread.c: Implementierung des Client-Threads
 */


#include "common/rfc.h"
#include "user.h"
#include "login.h"
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "catalog.h"
#include <time.h>
#include "common/util.h"
#include "score.h"



void *client_thread_main(int* client_id) {

	pthread_exit(0);
	return NULL;
}

int Timer(uint8_t* selection, int timeout, int sockD) {

	return 1;

}

