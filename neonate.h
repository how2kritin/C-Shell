#ifndef __NEONATE_H
#define __NEONATE_H

int max(int a, int b);
int latestPID();
void die(const char *s);
void disableRawMode();
void enableRawMode();
void neonate(char* token);

#endif
