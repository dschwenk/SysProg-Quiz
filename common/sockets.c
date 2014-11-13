/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Gemeinsam verwendete Module
 *
 * sockets.c: Implementierung der Funktionen zum Erstellen von Sockets
 *
 */

#include "sockets.h"

#include <string.h>
#include <unistd.h>



/**
 * öffnet Socket für den Server
 *
 * param port - Port des Sockets (normal 8111)
 *
 */
int openServerSocket(unsigned short port) {

	// Erstellt Socket auf IPv4
	int login_socket = socket(AF_INET, SOCK_STREAM, 0);

	// Fehler beim Socketerstellen abfangen
	if (login_socket == -1) {
		errorPrint("Socket konnte nicht erstellt werden: ");
		return -1;
	}

	// Struct fuer Socketadresse
	struct sockaddr_in address;

	// Addresseigenschaften
	// Erstellt Speicherplatz für Adresse und fuellt mit Nullen
	memset(&address, 0, sizeof(address));
	// IPv4 Adresse
	address.sin_family = AF_INET;
	// // konvertiere Werte von host byte order zu network byte order
	address.sin_port = htons(port);
	// Empfaengt von allen Interfaces
	address.sin_addr.s_addr = INADDR_ANY;

	// Socket an den Port binden
	if (bind(login_socket, (struct sockaddr *) &address, sizeof(address)) == -1) {
		errorPrint("bind error:");
		close(login_socket);
		return -1;
	}
	// testweise Serverport ausgeben
	infoPrint("Serverport: %d\n", port);
	return login_socket;
}

