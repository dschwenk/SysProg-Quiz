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

/**
 * öffnet Socket für den Server
 *
 * param port - Port des Sockets (normal 8111)
 *
 */

int openServerSocket(unsigned short port) {


        //Erstellt Socket auf IPv4
        int login_socket = socket(AF_INET, SOCK_STREAM, 0);

        //Fehler beim Socketerstellen abfangen
        if (login_socket == -1) {
                perror("Socket konnte nicht erstellt werden: ");
                return -1;
        }

        // Struct fuer Socketadresse
        struct sockaddr_in address;

        // Addresseigenschaften
        memset(&address, 0, sizeof(address)); // Erstellt Speicherplatz für Adresse und fuellt mit Nullen
        address.sin_family = AF_INET; // IPv4 Adresse
        address.sin_port = htons(port); // hton sorgt fuer Network-Byte-Order des Ports
        address.sin_addr.s_addr = INADDR_ANY; // Empfaengt von allen Interfaces

        // Socket an den Port binden
        if (bind(login_socket, (struct sockaddr *) &address, sizeof(address)) == -1) {
                perror("bind error:");
                close(login_socket);
                return -1;
        }
        // Gibt Serverport wieder
        infoPrint("Serverport: %d\n", port);
        return login_socket;
}

