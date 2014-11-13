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

// ?
PACKET activeCatalog;


/*
 * Funktion die einen Katalog zur Verwaltung hinzufuegt
 *
 * char* name
 * int i
 */
int addCatalog(char* name, int i) {
	// pruefe ob Katalognamen 'gueltig'
	debugPrint("pruefe Katalognamen\n.");
	if((name == NULL) || (strlen(name) == 0)){
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
 * int cSock
 */
int sendCatalog(int cSock) {
	PACKET send_catalog_packet;
	send_catalog_packet.header.type = RFC_CATALOGRESPONSE;
	// gehe alle verfuegbaren Kataloge durch
	for(int i = 0; i <= catalog_count; i++){
		// konvertiere Werte von host byte order zu network byte order
		send_catalog_packet.header.length = htons(strlen(catalog_array[i].CatalogName));
		// kopiere Katalogname in Paket
		strncpy(send_catalog_packet.content.catalogname, catalog_array[i].CatalogName,sizeof(catalog_array[i].CatalogName));
		debugPrint("Sende Katalog an Client.");
		sendPacket(send_catalog_packet, cSock);
	}
	// sende zum Abschluss einen leeren RFC_CATALOGRESPONSE --> alle Kataloge uebertragen
	send_catalog_packet.header.length = htons(0);
	strncpy(send_catalog_packet.content.catalogname, "", sizeof(""));
	debugPrint("Sende leeres Katalogpacket zum Abschluss an Client.");
	sendPacket(send_catalog_packet, cSock);
	return 1;
}



/*
 *
 * PACKET packet
 */
int setActiveCatalog(PACKET packet){
	activeCatalog = packet;
	// es wurde ein Katalog ausgewaehlt
	is_catalog_chosen = true;
	return 0;
}



// Sende Paket mit aktiven Katalog
PACKET getActiveCatalog(){
	return activeCatalog;
}



// gebe zurueck ob ein Katalog ausgewaehlt wurde
bool isCatalogChosen(){
	return isCatalogChosen;
}


/*
void setShMem(char* sh) {
	shmem = sh;
	return;
}

Question* getQuestion(int pos) {
	Question* QPtr = shmem + pos*(sizeof(Question));
	return QPtr;

}
*/
