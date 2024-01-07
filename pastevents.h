#ifndef __PASTEVENTS_H
#define __PASTEVENTS_H

void pastevents_handler(int* childPid, char* token);
int pastevents_retrieve(char* allCmds[16]);
void pastevents_add(char* input);
void pastevents_print();
char* pastevents_execute(char* token);
void pastevents_purge();

#endif
