#ifndef __SYSCMDS_H
#define __SYSCMDS_H

int processSysCmd(char* token);
int bringProcessToFG(char* cmdName, int childPID);

#endif