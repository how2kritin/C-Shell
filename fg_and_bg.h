#ifndef __FG_AND_BG_H
#define __FG_AND_BG_H

char* getCmdName(pid_t pid);
void fg(char* token);
void bg(char* token);

#endif