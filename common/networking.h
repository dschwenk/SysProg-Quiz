/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Gemeinsam verwendete Module
 *
 * netwokring.c: Implementierung der Funktionen zum Senden & Empfangen von Nachrichten
 *
 */

#ifndef NETWORKING_H
#define NETWORKING_H

#include "sockets.h"
#include "rfc.h"
#include "util.h"


void sendPacket(PACKET, int);
PACKET recvPacket (int);


#endif
