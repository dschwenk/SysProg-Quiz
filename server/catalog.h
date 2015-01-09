/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * catalog.h: Header für die Katalogbehandlung und Loader-Steuerung
 */

#ifndef CATALOG_H
#define CATALOG_H

#include "common/rfc.h"

#include <stdbool.h>


// enthaelt Namen der verfuegbaren Fragenkataloge
typedef struct CatalogArray {
	char CatalogName[31];
} CATALOGARRAY;


int addCatalog(char* name, int i);
int sendCatalog(int);
int setActiveCatalog(PACKET);
PACKET getActiveCatalog();
bool isCatalogChosen();
Question* getQuestion(int);
void setShMem(char*);


#endif
