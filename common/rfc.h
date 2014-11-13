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
#define MAX_PLAYERS	4;

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
#define RFC_ERRORWARNING         255 // Fehlermeldung an den Client


// selbst definierte Warnung-/Fehlertypen
#define ERR_SERVER_CLOSE 100
#define ERR_SERVER_MAXCOUNTPLAYERREACHED 101
#define ERR_SERVER_PLAYERNAMEEXIST 102


//Packen der Struckts auf minimale Größe
#pragma pack(push,1)


// Spelername + ID + Spielstand
typedef struct {
 	char playername[32];
 	uint32_t score;
 	uint8_t id;
} PLAYERLIST;


// Errortype + Errormessage
typedef struct {
	 uint8_t errortype;
	 char errormessage [100];
} ERROR;


// Antwortauswahl + richtige Antwort
typedef struct {
	uint8_t selection;
 	uint8_t correct;
} QUESTIONRESULT;


// Union, um alle Nachrichten in Content zu verpacken
typedef union {
	char playername[32];			// LoginRequest
	uint8_t clientid;				// LoginResponseok
	char catalogname[31];			// CatalogResponse, CatalogChange, StartGame
	PLAYERLIST playerlist[4];		// Playerlist
	QuestionMessage question;		// Question
	uint8_t selection;				// QuestionAnswered
	QUESTIONRESULT antwort;			// QuestionResult
	uint8_t playerrank;				// GameOver
	ERROR error;					// Error
} CONTENT;


// Header
typedef struct {
	uint8_t type;
	uint16_t length;
} HEADER;
// HEADER head;


// Paket - Header + Nachricht
typedef struct{
	HEADER header;
	CONTENT content;
} PACKET;

#pragma pack(pop)


#endif
