/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Gemeinsam verwendete Module
 *
 * sockets.c: Implementierung der Funktionen zum Erstellen von Sockets
 *
 */

#ifndef SOCKETS_H
#define SOCKETS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "networking.h"



int openServerSocket(unsigned short);


#endif
