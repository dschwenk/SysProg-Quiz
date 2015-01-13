/*
 * rfc.c
 *
 *  Created on: Jan 13, 2015
 *      Author: dschwenk
 */


#include "rfc.h"

/*
 * Funktion prueft ob der Typ im Header dem
 * uebergeben String entspricht
 * Gibt bei Uebereinstimmung 1 zurueck, sonst 0
 */
int isStringEqual(HEADER m, const char *s) {
    if ((m.type[0] == s[0]) && (m.type[1] == s[1]) && (m.type[2] == s[2])){
        return 1;
    }
    return 0;
}
