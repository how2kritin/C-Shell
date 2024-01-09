#ifndef HEADERS_H_
#define HEADERS_H_

// System headers (Pre-existing headers)
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <termios.h>
#include <ctype.h>

// Custom-made headers
#include "prompt.h"
#include "../custom-commands/warp.h"
#include "../custom-commands/peek.h"
#include "../custom-commands/pastevents.h"
#include "syscmds.h"
#include "../custom-commands/proclore.h"
#include "../custom-commands/seek.h"
#include "bgHandler.h"
#include "../custom-commands/signals.h"
#include "../networking/iMan.h"
#include "../custom-commands/neonate.h"
#include "../custom-commands/fg_and_bg.h"

// General global macro definitions and constants:
#define MAX_CONCURRENT_BG_PROCESSES 500
#define MAX_FILES_IN_HISTORY 15
#define MAX_ARGS 512
#define MAX_PIPED_CMDS 512

// Colors: Reference -> https://gist.github.com/RabaDabaDoba/145049536f815903c79944599c6f952a

#define CRESET "\033[0m"

// Normal colors:
#define BLK "\033[0;30m"
#define RED "\033[0;31m"
#define GRN "\033[0;32m"
#define YEL "\033[0;33m"
#define BLU "\033[0;34m"
#define MAG "\033[0;35m"
#define CYN "\033[0;36m"
#define WHT "\033[0;37m"

// Bold colors:
#define BBLK "\033[1;30m"
#define BRED "\033[1;31m"
#define BGRN "\033[1;32m"
#define BYEL "\033[1;33m"
#define BBLU "\033[1;34m"
#define BMAG "\033[1;35m"
#define BCYN "\033[1;36m"
#define BWHT "\033[1;37m"

// Extern declaration of global variables: Reference -> https://stackoverflow.com/questions/1433204/how-do-i-use-extern-to-share-variables-between-source-files
extern char homeDirAbsPath[PATH_MAX];
extern char prevDir[PATH_MAX];
extern char delimiters[3];
extern bool isForegroundRunning;
extern int currNumBGProcesses;

// main function declarations:
void runCmd(int* childPid, char* token);
int splitCmds(const char input[], char* allCmds[]);

#endif