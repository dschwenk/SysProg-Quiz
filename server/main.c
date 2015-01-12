/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * main.c: Hauptprogramm des Servers
 */

#include "main.h"
#include "common/util.h"
#include "common/rfc.h"
#include "login.h"
#include "user.h"
#include "score.h"
#include "catalog.h"
#include "common/sockets.h"
#include "common/networking.h"
#include "common/server_loader_protocol.h"

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
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>



// Serverport
int server_port;
// Serversocket
int server_socket;
// Server singleton file
int SingleInstanceFile = 0;


// Name von loader
const char *program_name = "loader";
// PID Loader
pid_t forkResult;
// Groesse Shared Memory
int shmLen = 0;
// Handle fuer Shared Memory
int shmHandle;
// Shared Memory Mapping
char* shmData;
// Pipes zum Austausch zwischen Server und Loader
int stdoutPipe[2];
int stdinPipe[2];

// Katalogverzeichnis
char* catalog_dir = { "../catalog" };


/*
 * Funktion setzt den Serverport
 */
void set_port(char* port_str){
	int port = atoi(port_str);
	if((port < 65535) && (port > 0)){
		server_port = port;
	}
	else {
		infoPrint("Port muss zwischen 1 - 65535 sein!");
		infoPrint("Es wird der Standardport 8111 verwendet");
		server_port = 8111;
	}
}


/*
 * Funktion gibt den Serverport zurueck
 */
int get_port(){
	return server_port;
}

/*
 * Funktion gibt Hilfe aus
 */
void show_help() {
    printf("Available options:\n");
    printf("    -p --port       specify a port (argument)\n");
    printf("    -c --catalog    specify a catalog folder (argument)\n");
    printf("    -v --verbose    enable debug output\n");
    printf("    -h --help       show this help message\n");
}


/**
 * Funktion wertet Startparameter aus
 * param argc - Anzahl der Startparameter
 * param argv - Startparameter
 */
void process_commands(int argc, char** argv) {

    debugPrint("Parsing command line options...");

	// Standard Port 8111
	char* server_port = { "8111" };

    const char* short_options = "hvp:c";
    struct option long_options[] = {
        { "help", no_argument, 0, 'h' },
        { "verbose", no_argument, 0, 'v' },
        { "port", required_argument, 0, 'p' },
        { "catalog", no_argument, 0, 'c' },
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
		    	infoPrint("debug Ausgabe aktiviert.");
				break;
			// Port
			case 'p':
				server_port = optarg;
				break;
			// Katalog
			case 'c':
				catalog_dir = optarg;
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
		debugPrint("Cannot create PID file");
		exit(1);
    }

    debugPrint("Acquiring file write lock");
    /* Important: lock before doing any IO */
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    // fcntl - manipulate file descriptor
    // F_GETLK, F_SETLK and F_SETLKW are used to acquire, release, and test for the existence of record locks
    if (fcntl(file, F_SETLK, &lock) < 0) {
    	infoPrint("Server läuft bereits..");
    	exit(1);
    }

    // truncate - kuerzen
    // ftruncate, truncate - truncate a file to a specified length
    if (ftruncate(file, 0) < 0) {
    	infoPrint("Server läuft bereits..");
    	exit(1);
    }

    // hole PID des servers
    int pid = getpid();
    if (write(file, &pid, sizeof(pid)) < sizeof(pid)) {
    	debugPrint("write");
    }

    if (fsync(file) < 0) {
    	debugPrint("fsync");
    }
}


// Schliesse SingleInstance Datei (bei Spielende)
void closeSingleInstance(int file){
	close(file);
}


/*
 * Funktion schliesst alle verwendeten Ressourcen und
 * beendet den Server
 */
void endServer(){

	debugPrint("Beende Server.");

	// Nachricht an alle Clients senden - sofern welche angemeldet
	debugPrint("Sende Nachricht an Clients: Server wird beendet.");
	if(countUser() > 0){
		PACKET close_server_packet;
		close_server_packet.header.type = RFC_ERRORWARNING;
		close_server_packet.header.length = htons(sizeof(ERROR));
		//close_server_packet.content.error.errortype = ERR_SERVER_CLOSE;
		close_server_packet.content.error.errortype = ERR_FATAL;
		strncpy(close_server_packet.content.error.errormessage, "Server beendet", 100);
		// sende Nachricht
		sendToAll(close_server_packet);
		debugPrint("Nachricht ueber Serverende an alle Clients verschickt.");
	}

	// Socket schliessen
	if(close(server_socket) == 0){
		debugPrint("Serversocket geschlossen.");
	}
	else {
		debugPrint("Konnte Serversocket nicht schliessen.");
	}

	// shared memorey
	// aus Adressraum entfernen
	munmap(shmData, shmLen);
	// Filedeskritpor SharedMemory schliessen
	close(shmHandle);
	// SharedMemory Objekt entfernen
	shm_unlink(SHMEM_NAME);
	debugPrint("Shared Memory entfernt.");

	// loader beenden
	if(kill(forkResult, SIGINT) == 0){
		debugPrint("Loaderprozess beendet.");
	}
	else {
		debugPrint("Loaderprozess konnte nicht beendet werden.");
	}

	// SingleInstance-Datei schliessen und loeschen
	closeSingleInstance(SingleInstanceFile);
	if(remove("serverInstancePIDFile") == 0){
		debugPrint("SingleInstanceFile geschlossen und geloescht.");
	}
	else {
		debugPrint("Konnte SingleInstanceFile nicht loeschen.");
	}

	infoPrint("bye ...");
	exit(0);
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
	infoPrint("\n\nServer Gruppe Joos, Frick & Schwenk\n");

	// Spielerverwaltung initialisieren
	initSpielerverwaltung();

	// Socket erstellen
	server_socket = openServerSocket(server_port);
	if(server_socket == -1){
		errorPrint("Server Socket konnte nicht erstellt werden!");
		endServer();
		exit(0);
	}

	// Loader starten + Kataloge laden
	int loader_start_result = startLoader();
	if(loader_start_result != 0){
		errorPrint("Fehler beim Starten des Loaders: %i", loader_start_result);
		endServer();
		exit(0);
	}
	int loader_load_catalogs_result = loadCatalogs();
	if(loader_load_catalogs_result != 0){
		errorPrint("Fehler beim Laden der Kataloge: %i", loader_load_catalogs_result);
		endServer();
		exit(0);
	}

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


/*
 * Funktion startet den Loader als Kindprozess
 */
int startLoader(){
	// Pipes erzeugen
	if(pipe(stdinPipe) == -1 || pipe(stdoutPipe) == -1){
		errorPrint("Konnte Pipe nicht erzeugen");
		return 3;
	}
	// Kindprozess abspalten
	forkResult = fork();
	if(forkResult < 0){
		errorPrint("Konnte Kindprozess nicht abspalten");
		return 1;
	}
	else if (forkResult == 0){
		// im Kindprozess

		/* Umleitung der Standardeingabe
		 *
		 * damit der Kindprozess anstelle der Konsole die Pipes als Standardein / ausgabe
		 * verwendet, nutzen wir dup2 - duplicate a file descriptor
		 *
		 * int dup2(int oldfd, int newfd);
		 * Nach dem Aufruf ist oldfd zusätzlich auch als newfd ansprechbar.
		 *
		 * Übergeben wir als oldfd das Leseende der Pipe für die Standard-
		 * eingabe (stdinPipe[0]) und als die Konstante newfd STDIN_FILENO (=0), so
		 * wird von nun an die Pipe als Standardeingabe für den aktuellen Prozess verwendet.
		 */
		if (dup2(stdinPipe[0], STDIN_FILENO) == -1) {
			errorPrint("dup2(stdinPipe[0], STDIN_FILENO)");
			return 4;
		}

		// Umleitung der Standardausgabe
		if (dup2(stdoutPipe[1], STDOUT_FILENO) == -1) {
			errorPrint("dup2(stdoutPipe[1], STDOUT_FILENO)");
			return 5;
		}

		/* Schliessen aller Pipe-Deskriptoren ->
		 * Die unnoetig offenen Filedeskriptoren auf die Schreibenden der Pipes
		 * verhindern, dass die Prozesse sich gegenseitig über das Ende der
		 * Datenübertragung benachrichtigen können, denn das Dateiende einer Pipe
		 * gilt genau dann als erreicht, wenn alle Filedeskriptoren für das
		 * Schreibende geschlossen wurden.
		 */
		close(stdinPipe[0]);
		close(stdoutPipe[1]);
		close(stdinPipe[1]);
		close(stdoutPipe[0]);

		// Anderes Programm in die vorbereitete Prozessumgebung laden
		// param -d Loader Debugausgabe
		// param Katalogverzeichnis
		execl(program_name, program_name, "-d", catalog_dir, NULL); /* Neues Programm läuft... */
		errorPrint("exec error"); /* ...oder auch nicht, dann war's aber ein Fehler */
	}
	else {
		/* im Elternprozess */

		// Schliessen der hier nicht benoetigten Enden der Pipes
		close(stdinPipe[0]);
		close(stdoutPipe[1]);
	}
	return 0;
}


/*
 * Funktion liesst Kataloge aus
 */
int loadCatalogs(){

	// BROWSE_CMD - Kataloge auflisten
	if(write(stdinPipe[1], BROWSE_CMD, strlen(BROWSE_CMD)) < strlen(BROWSE_CMD)){
		errorPrint("Senden der Nachricht über Pipe fehlgeschlagen: ");
		return 1;
	}

	// Zeilenumbruch
	if(write(stdinPipe[1], "\n", 1) < 1){
		errorPrint("Senden der Nachricht über Pipe fehlgeschlagen: ");
		return 2;
	}

	// Katalogname
	char* catalogname;
	// Zaehlvariable fuer Anzahl Kataloge
	int i = 0;
	int err = 0;

	while(err > -1){
		// Daten aus dem Leseende von stdoutPipe lesen
		catalogname = readLine(stdoutPipe[0]);
		// pruefe ob Zeilenumbruch
		err = strcmp(catalogname, "\n");
		if(err > -1){
			// Katalog hinzufuegen
			addCatalog(catalogname, i);
			i++;
		}
	}
	infoPrint("Kataloge eingelesen: %i", i);
	return 0;
}


/*
 *
 *
 */
void loadQuestions(char* name){
	char* message;
	int loaded;

	// remove shared memorey
	shm_unlink(SHMEM_NAME);

	// LOAD_CMD_PREFIX - Katalog laden
	if(write(stdinPipe[1], LOAD_CMD_PREFIX, strlen(LOAD_CMD_PREFIX)) < strlen(LOAD_CMD_PREFIX)){
		errorPrint("Senden der Nachricht über Pipe fehlgeschlagen");
		endServer();
		exit(0);
	}
	// aktiven Katalog uebergeben
	if(write(stdinPipe[1], name, strlen(name)) < strlen(name)){
		errorPrint("Senden der Nachricht über Pipe fehlgeschlagen");
		endServer();
		exit(0);
	}
	// Zeilenumbruch zum Abschluss
	if (write(stdinPipe[1], "\n", 1) < 1) {
		errorPrint("Senden der Nachricht über Pipe fehlgeschlagen");
		endServer();
		exit(0);
	}

	// von Pipe lesen ob laden des Katalogs erfolgreich (LOAD_SUCCESS_PREFIX - Katalog mit SIZE Fragen geladen)
	message = readLine(stdoutPipe[0]);
	if(strcmp(message, LOAD_SUCCESS_PREFIX) != -1){
		infoPrint("Kataloge eingelesen");
		// The memmove() function copies n bytes from memory area src to memory area dest
		memmove(message, message + sizeof(LOAD_SUCCESS_PREFIX) - 1, 50);
		loaded = atoi(message);
		infoPrint("Anzahl an eingelesenen Fragen: %i", loaded);
	}
	else {
		errorPrint("Fragen konnten nicht geladen werden");
		endServer();
		exit(0);
	}

	// shm_open() creates and opens a new, or opens an existing, POSIX shared memory object.
	// returns file descriptor of -1 in case of error
	shmHandle = shm_open(SHMEM_NAME, O_RDONLY, 0);
	if(shmHandle == -1){
		errorPrint("Konnte Shared Memory nicht erstellen / öffnen!");
		endServer();
		exit(0);
	}

	// Groesse ShareMemory = Anzahl Fragen * Groesse der Fragen
	shmLen = loaded * sizeof(Question);
	ftruncate(shmHandle, shmLen);

	/*
	 * Um ein mittels shm_open geöffnetes Shared Memory Objekt nun tatsächlich verwenden zu können, muss
	 * dieses in den Adressraum des Prozesses eingebunden werden.
	 * Die Funktion mmap bindet eine Datei, ein Gerät oder ein Shared Memory Objekt in den Adressraum
	 * des aufrufenden Prozesses ein.
	 */
	// mmap() creates a new mapping in the virtual address space of the calling process.
	// void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
	shmData = mmap(NULL, shmLen, PROT_READ, MAP_SHARED, shmHandle, 0);
	if(shmData == MAP_FAILED){
		errorPrint("Konnte Shared Memory nicht in Adressraum einbinden!");
		endServer();
		exit(0);
	}

	//  SharedMemory
	setShMem(shmData);

	return;
}

