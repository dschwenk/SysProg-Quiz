/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Gemeinsam verwendete Module
 * 
 * rfc.h: Definitionen für das Netzwerkprotokoll gemäß dem RFC
 */

#ifndef RFC_H
#define RFC_H

#include "common/question.h"

// maximale Anzahl an Spielern
#define MAX_PLAYERS	4

// RFC Version WS 14/15
#define RFC_VERSION 6

#define ERR_WARNING 0
#define ERR_FATAL 1


// max. Laenge Spielername (inkl. '\0')
#define PLAYER_NAME_LENGTH 32
// max. Laenge Katalogname
#define MAX_CATALOG_NAME_LENGTH 31
// define max. Messagelength (inkl. '\0')
#define MAX_MESSAGE_LENGTH 100


//Packen der Struckts auf minimale Größe - keine Fuellbytes
#pragma pack(push,1)


// LoginRequest
typedef struct {
	uint8_t RFCVersion;
	char name[31]; // nicht nullterminiert
} LOGINREQUEST;

// LoginResponseOK
typedef struct {
	uint8_t RFCVersion;
	uint8_t clientid;
} LOGINRESPONSEOK;

// Spelername + ID + Spielstand
typedef struct {
 	char playername[PLAYER_NAME_LENGTH];
 	uint32_t score;
 	uint8_t id;
} PLAYERLIST;

// Errortype + Errormessage
typedef struct {
	 uint8_t subtype; // 0 Warning, 1 Fatal -> exit Client
	 char message[MAX_MESSAGE_LENGTH]; // nicht nullterminiert
} ERROR;

// Antwortauswahl + richtige Antwort
typedef struct {
	uint8_t timeout; // wenn Timout fuer Frage erreicht wurde ungleich 0, sonst 0
 	uint8_t correct; // Bitmaske der richtigen Antwortmoeglichkeiten
} QUESTIONRESULT;


// Union, um alle Nachrichten in Content zu verpacken
typedef union {
	LOGINREQUEST loginrequest;		// LRQ - LoginRequest
	LOGINRESPONSEOK loginresponseok;	// LOK - LoginResponseok
	char catalogname[MAX_CATALOG_NAME_LENGTH];	// CRQ, CCH, STG - CatalogRequest, CatalogChange, StartGame
	PLAYERLIST playerlist[MAX_PLAYERS];	// LST - Playerlist
	QuestionMessage question;		// QUE - Question
	uint8_t selection;			// QAN - QuestionAnswered
	QUESTIONRESULT questionresult;		// QRE - QuestionResult
	uint8_t playerrank;			// GOV - GameOver
	ERROR error;				// ERR - Error
} CONTENT;


// Header
typedef struct {
	uint8_t type[3];
	uint16_t length;
} HEADER;


// Paket - Header + Nachricht
typedef struct{
	HEADER header;
	CONTENT content;
} PACKET;

#pragma pack(pop)



// Funktion prueft ob der Typ im Header dem uebergeben String entspricht
// gibt bei Uebereinstimmung 1 zurueck falls nicht 0
int isStringEqual(HEADER, const char *s);



// selbst definierte Warnung-/Fehlertypen + Konstanten ('ersetzen ERR_WARNUNG / FATAL)
#define ERR_MAXCOUNTPLAYERREACHED 1
#define ERR_PLAYERNAMEEXIST 1
#define ERR_GAMEISRUNNING 1
#define ERR_SPIELLEITERLEFTGAME 1
#define ERR_TOOFEWPLAERS_GAME 1

#define ERR_TOOFEWPLAERS_PREP 0
#define ERR_CLIENTLEFTGAME 0

#endif
