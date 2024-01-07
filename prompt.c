#include "headers.h"

// References: To get username in C/C++ in Linux: https://stackoverflow.com/questions/8953424/how-to-get-the-username-in-c-c-in-linux AND man getlogin
// To get system name in Linux: https://stackoverflow.com/questions/27914311/get-computer-name-and-logged-user-name AND man gethostname
// To get the current working directory in Linux: https://stackoverflow.com/questions/298510/how-to-get-the-current-directory-in-a-c-program and man getcwd

// Colors: https://www.theurbanpenguin.com/4184-2/

void getrelpath(char* relPath, char* currDir){
    int prefixLen = strlen(homeDirAbsPath);
    if(strncmp(homeDirAbsPath, currDir, prefixLen) == 0){ // if the currDir path has the homeDirAbsPath as prefix:
        relPath[0] = '~';
        int relPathidx = 1;
        for(int i = prefixLen; currDir[i] != '\0'; i++){
            relPath[relPathidx++] = currDir[i];
        }
        relPath[relPathidx] = '\0';
    }
    else strcpy(relPath, currDir);
}

void prompt(char* token, time_t totalTime) {
    char* username = getlogin();
    char systemName[HOST_NAME_MAX]; // Max host name in linux is 256 bytes only, however it is system dependent. So, I am using this macro from limits.h.
    char currDir[PATH_MAX];
    getcwd(currDir, PATH_MAX);
    char relPath[PATH_MAX]; // To handle relative path
    getrelpath(relPath, currDir);
    gethostname(systemName, HOST_NAME_MAX);
    printf(WHT "<" BCYN "%s" WHT "@" GRN "%s" WHT ":" RED "%s", username, systemName, relPath);
    if(totalTime > 2) printf(YEL " %s" WHT " : " MAG "%lds", token, totalTime);
    printf(WHT "> " CRESET);
}
