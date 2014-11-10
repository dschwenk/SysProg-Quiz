/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * main.c: Hauptprogramm des Servers
 */

#include "common/util.h"
#include "common/rfc.h"
#include "login.h"
#include "user.h"
#include "score.h"
#include "catalog.h"
#include "common/sockets.h"
#include "common/networking.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>



// Serverport
int server_port;
// Serversocket
int server_socket;
// Server singleton file
int SingleInstanceFile = 0;


void set_port(char* port_str){
	server_port = atoi(port_str);
}

int get_port(){
        return server_port;
}

// gebe Hilfe aus
void show_help() {
    printf("Available options:\n");
    printf("    -p --port       specify a port (argument)\n");
    printf("    -v --verbose    enable debug output\n");
    printf("    -h --help       show this help message\n");
}


/**
 * Verarbeite Startparameter
 * param argc - Anzahl der Startparameter
 * param argv - Startparameter
 */

void process_commands(int argc, char** argv) {

    debugPrint("Parsing command line options...");

	// Standard Port 8111
	char* server_port = { "8111" };

    const char* short_options = "hvp:";
    struct option long_options[] = {
        { "help", no_argument, 0, 'h' },
        { "verbose", no_argument, 0, 'v' },
        { "port", required_argument, 0, 'p' },
        { NULL, 0, NULL, 0 }
    };

    int opt_index = 0;
    int loop = 1;
    while(loop != 0){
    	int opt = getopt_long(argc, argv, short_options, long_options, &opt_index);
    	switch(opt){
			// zeige Hilfe
    		case 'h':
				show_help();
				exit(1);
				break;
			// Verbose
			case 'v':
				debugEnable();
		    	infoPrint("debug Ausgabe aktiviert.\n");
				break;
			// Port
			case 'p':
				server_port = optarg;
				break;
			case -1:
				loop = 0;
				break;
			// Option nicht bekannt
			default:
				break;
			}
    	}

	// Setze Port
	set_port(server_port);
}



// Der Server soll auf dem lokalen System nur einmal gestartet werden koennen.
void setSingleInstance(int file){

	// lege Datei an
    file = open("serverInstancePIDFile", O_WRONLY| O_CREAT, 0644);

    struct flock lock;

    // pruefe ob Datei anlegen erfolgreich
    if (file < 0) {
		debugPrint("Cannot create PID file\n");
		exit(1);
    }

    debugPrint("Acquiring file write lock\n");
    /* Important: lock before doing any IO */
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    // fcntl - manipulate file descriptor
    // F_GETLK, F_SETLK and F_SETLKW are used to acquire, release, and test for the existence of record locks
    if (fcntl(file, F_SETLK, &lock) < 0) {
    	infoPrint("Server läuft bereits..\n");
    	exit(1);
    }

    // truncate - kuerzen
    // ftruncate, truncate - truncate a file to a specified length
    if (ftruncate(file, 0) < 0) {
    	infoPrint("Server läuft bereits..\n");
    	exit(1);
    }

    // hole
    int pid = getpid();
    if (write(file, &pid, sizeof(pid)) < sizeof(pid)) {
    	debugPrint("write\n");
    }

    if (fsync(file) < 0) {
    	debugPrint("fsync\n");
    }
}


// Schliesse SingleInstance Datei (bei Spielende)
void closeSingleInstance(int file){
	close(file);
}


// hinterlasse nach beenden ein sauberes System
void endServer(){

	debugPrint("Beende Server.");

	// SingleInstance-Datei schliessen und loeschen
	closeSingleInstance(SingleInstanceFile);
	remove("serverInstancePIDFile");
	debugPrint("SingleInstanceFile geschlossen und geloescht.");

	// Socket schliessen
	close(server_socket);
	debugPrint("Serversocket geschlossen.");

	// TODO
	// loader beenden, shared memorey

	infoPrint("bye ...");
}


/*
 * Singnal handler
 * http://www.csl.mtu.edu/cs4411.ck/www/NOTES/signal/install.html
 */
void INThandler(int sig) {
	char c = '0';
	if(sig != SIGINT){
		debugPrint("Signal is not SIGINT.");
		return;
	}
	else {
		infoPrint("Wollen Sie den Server wirklich beenden? [j/n]: ");
		c = getchar();
		if((c == 'j') || (c == 'J')){
			debugPrint("Server soll beendet werden.");
			endServer();
			exit(0);
		}
		else {
			debugPrint("Server soll nicht beendet werden.");
			return;
		}
	}
}


/**  Start des Servers
 *   param argc Anzahl der Startparameter
 *   param argv Startparameter
 */

int main(int argc, char ** argv) {

	// Der Server soll auf dem lokalen System nur einmal gestartet werden koennen.
	setSingleInstance(SingleInstanceFile);

	setProgName(argv[0]);

	// verarbeite Parameter
	process_commands(argc, argv);

	// reagiere auf Signal, u.a. STRG + C
	signal(SIGINT, INThandler);

	// gebe Gruppennamen aus
	infoPrint("Server Gruppe Jost, Frick & Schwenk\n");

	// Spielerverwaltung initialisieren
	initSpielerverwaltung();

	// Socket erstellen
	server_socket = openServerSocket(server_port);
	if(server_socket == -1){
		errorPrint("Server Socket konnte nicht erstellt werden!");
		endServer();
		exit(0);
	}

	// TODO
	// Loader starten


	// http://pubs.opengroup.org/onlinepubs/7908799/xsh/sem_init.html
	// Semaphor fuer Spielerliste / Punktestand initialisieren
	sem_init(&semaphor_score, 0, 0);

	// Score thread starten
	pthread_t score_thread;
	if(pthread_create(&score_thread, NULL, (void*)&score_main, NULL) == -1){
		errorPrint("Score_thread konnte nicht erstellt werden!");
		endServer();
		exit(0);
	}

	// starte Login-Thread
	pthread_t login_thread;
	if(pthread_create(&login_thread, NULL, (void*)login_main(server_socket), NULL) == -1){
		errorPrint("Login_thread konnte nicht erstellt werden!");
		endServer();
		exit(0);
	}



	// beende Server
	endServer();

	return 0;
}
