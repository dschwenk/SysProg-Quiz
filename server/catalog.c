/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * catalog.c: Implementierung der Fragekatalog-Behandlung und Loader-Steuerung
 */

#include "common/util.h"
#include "common/server_loader_protocol.h"
#include "common/networking.h"
#include "common/rfc.h"
#include "catalog.h"
#include "user.h"

#include <stdbool.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


// Array fuer die Katalogverwaltung
CATALOGARRAY catalog_array[20];

// Anzahl an Katalogen - Zaehler
int catalog_count = 0;

// bool um zu bestimmen ob vom Spielleiter ein Katalog ausgewaehlt wurde
bool is_catalog_chosen = false;

// Paket enthaelt Namen des aktiven / aktuellen Katalogs
PACKET activeCatalog;

// String Shared-Memory 'MappingName'
char* shmem;


/*
 * Funktion die einen Katalog zur Verwaltung hinzufuegt
 *
 * param char* name Name des Katalogs
 * param int i Zaehler / Katalognummer
 */
int addCatalog(char* name, int i){
	// pruefe ob Katalognamen 'gueltig'
	debugPrint("pruefe Katalognamen\n.");
	if((name == NULL) || (strlen(name) == 0 || name == "\n")){
		return -1;
	}
	// kopiere Katalognamen in Katalogverwaltungsarray
	strncpy(catalog_array[i].CatalogName, name, strlen(name));
	// erhoehe Katalogzaehler
	if(i > catalog_count){
		catalog_count = i;
	}
	debugPrint("Katalog hinzugefuegt.\n");
	return 0;
}


/*
 * Funktion um verfuegbare Kataloge an Client zu senden
 *
 * int client_socket Socketdeskriptor des Clients
 */
int sendCatalog(int client_socket){
	PACKET send_catalog_packet;
	send_catalog_packet.header.type = RFC_CATALOGRESPONSE;
	// gehe alle verfuegbaren Kataloge durch
	for(int i=0;i<= catalog_count;i++){
		// konvertiere Werte von host byte order zu network byte order
		send_catalog_packet.header.length = htons(strlen(catalog_array[i].CatalogName));
		// kopiere Katalogname in Paket
		strncpy(send_catalog_packet.content.catalogname, catalog_array[i].CatalogName,sizeof(catalog_array[i].CatalogName));
		debugPrint("Sende Katalog an Client.");
		sendPacket(send_catalog_packet, client_socket);
	}
	// sende zum Abschluss einen leeren RFC_CATALOGRESPONSE --> alle Kataloge uebertragen
	/*
	send_catalog_packet.header.length = htons(0);
	strncpy(send_catalog_packet.content.catalogname, "", sizeof(""));
	debugPrint("Sende leeres Katalogpacket zum Abschluss an Client.");
	sendPacket(send_catalog_packet, client_socket);
	*/
	return 1;
}


/*
 * Funktion setzt den aktiven / gewaehlten Katalog
 * PACKET packet enthaelt den aktuell gewaehlten Fragenkatalog
 */
int setActiveCatalog(PACKET packet){
	activeCatalog = packet;
	// es wurde ein Katalog ausgewaehlt
	is_catalog_chosen = true;
	return 0;
}


/*
 * Funktion gibt den aktuellen Katalog zurueck
 */
PACKET getActiveCatalog(){
	return activeCatalog;
}


/*
 * Funktion gibt zurueck ob ein Katalog ausgewaehlt wurde
 */
bool isCatalogChosen(){
	return is_catalog_chosen;
}


/*
 * Funktion setzt den Shared-Memory 'MappingName'
 */
void setShMem(char* sh){
	shmem = sh;
	return;
}


/*
 * Funktion liesst von SharedMemory Fragen
 */
Question* getQuestion(int pos){
	Question* question = shmem + pos*(sizeof(Question));
	return question;
}

