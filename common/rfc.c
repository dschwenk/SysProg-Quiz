/*
 * rfc.c
 *
 *  Created on: Jan 13, 2015
 *      Author: dschwenk
 */


#include "rfc.h"

int equalLiteral(HEADER m, const char *s) {
    if ((m.type[0] == s[0])
            && (m.type[1] == s[1])
            && (m.type[2] == s[2])) {
        return 1;
    }
    return 0;
}
