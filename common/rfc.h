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


// Funktion prueft ob der Typ im Header dem uebergeben String entspricht
// gibt bei Uebereinstimmung 1 zurueck falls nicht 0
int isStringEqual(struct rfcMain m, const char *s);

// maximale Anzahl an Spielern
#define MAX_PLAYERS	4

// RFC Version WS 14/15
#define RFC_VERSION 6

/*
// Uebersicht über die Nachrichtentypen
#define RFC_LOGINREQUEST         1 // Anmeldung eines Clients am Server
#define RFC_LOGINRESPONSEOK      2 // Anmeldung am Server erfolgreich
#define RFC_CATALOGREQUEST       3 // Anforderung der Liste der Fragakata-loge durch den Client
#define RFC_CATALOGRESPONSE      4 // Name eines Fragekatalogs (mehrere Nachrichten dieses Typs ergeben die vollständige Liste)
#define RFC_CATALOGCHANGE        5 // Spielleiter hat Katalogauswahl geaendert, wird an alle Clients weitergeleitet
#define RFC_PLAYERLIST           6 // Liste der Spielteilnehmer, wird versendet bei: An-/Abmeldung, Spielstart und Aenderung des Punktestandes
#define RFC_STARTGAME            7 // Spielleiter möchte Spiel starten, wird vom Server ausgewertet und an Clients weitergeleitet
#define RFC_QUESTIONREQUEST      8 // Anforderung einer Quizfrage durch einen Client
#define RFC_QUESTION             9 // Reaktion auf QuestionRequest: Transport einer Quiz-Frage zum Client
#define RFC_QUESTIONANSWERED     10 // Quiz-Frage wurde beantwortet
#define RFC_QUESTIONRESULT       11 // Auswertung einer Antwort auf eine Quiz-Frage
#define RFC_GAMEOVER             12 // Alle Clients sind fertig, Mitteilung ueber Endplatzierung
#define RFC_ERRORWARNING         255 // Fehlermeldung
*/

#define ERR_WARNING 0
#define ERR_FATAL 1

// selbst definierte Warnung-/Fehlertypen + Konstanten ('ersetzen ERR_WARNUNG / FATAL)
#define ERR_MAXCOUNTPLAYERREACHED 101
#define ERR_PLAYERNAMEEXIST 102
#define ERR_GAMEISRUNNING 103
#define ERR_SPIELLEITERLEFTGAME 104
#define ERR_TOOFEWPLAERS 105
#define ERR_CLIENTLEFTGAME 107

// max. Laenge Spielername (inkl. '\0')
#define PLAYER_NAME_LENGTH 32
// max. Laenge Katalogname
#define CATALOG_NAME_LENGTH 31
// define max. Messagelength (inkl. '\0')
#define MAX_MESSAGE_LENGTH 100


//Packen der Struckts auf minimale Größe - keine Fuellbytes
#pragma pack(push,1)


// LoginRequest
typedef struct {
	uint8_t RFCVersion;
	char playername[PLAYER_NAME_LENGTH];
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
	 uint8_t errortype;
	 char errormessage[MAX_MESSAGE_LENGTH];
} ERROR;

// Antwortauswahl + richtige Antwort
typedef struct {
	uint8_t timeout;
 	uint8_t correct;
} QUESTIONRESULT;


// Union, um alle Nachrichten in Content zu verpacken
typedef union {
	LOGINREQUEST loginrequest;		// LRQ - LoginRequest
	LOGINRESPONSEOK loginresponseok;	// LOK - LoginResponseok
	char catalogname[CATALOG_NAME_LENGTH];	// CRQ, CCH, STG - CatalogRequest, CatalogChange, StartGame
	PLAYERLIST playerlist[MAX_PLAYERS];	// LST - Playerlist
	QuestionMessage question;		// QUE - Question
	uint8_t selection;			// QAN - QuestionAnswered
	QUESTIONRESULT questionresult;		// QRE - QuestionResult
	uint8_t playerrank;			// GOV - GameOver
	ERROR error;				// ERR - Error
} CONTENT;


// Header
typedef struct {
	char type[3];
	uint16_t length;
} HEADER;


// Paket - Header + Nachricht
typedef struct{
	HEADER header;
	CONTENT content;
} PACKET;

#pragma pack(pop)


#endif
