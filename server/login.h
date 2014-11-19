/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * login.h: Header für das Login
 */

#ifndef LOGIN_H
#define LOGIN_H

#include <stdbool.h>

void* login_main(int);
void setGameRunning();
bool getGameRunning();

#endif
