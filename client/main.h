/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Client
 *
 * main.h: Header für Main
 */

#ifndef MAIN_H
#define MAIN_H

int establishConnection(int , char*, char*);
void loginRequest(char*);
void catalogRequest();
void show_Clienthelp();
void process_client_commands(int, char**);
int main(int, char **);

void preparation_onCatalogChanged(const char *);

void preparation_onStartClicked(const char *);

void preparation_onWindowClosed(void);

void game_onAnswerClicked(int);


void game_onSubmitClicked(unsigned char);


void game_onWindowClosed(void);


#endif

