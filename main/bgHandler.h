#ifndef __BGHANDLER_H
#define __BGHANDLER_H

void addBackgroundProcess(char* token, int childPid);
int processBGSIGCHLD();
void removeProcess(int childPid);
bool isProcessInList(pid_t pid);
void activities();
void ctrlDInterrupt();

#endif
