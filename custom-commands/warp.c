#include "../main/headers.h"

// To tokenize strings: strtok -> Reference: man strtok AND https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm

static bool atleastOneDirVisited = false;
char prevDir[PATH_MAX], currDir[PATH_MAX];

void changeDir(const char* finalPath){
    int returnVal = chdir(finalPath);
    if(returnVal == -1) {
        fprintf(stderr, BRED "ERROR: warp command failed for path \"%s\".\nKindly ensure that a valid directory exists at that path, and that you have sufficient permissions to access it!\n" CRESET, finalPath);
        return;
    }
    strcpy(prevDir, currDir);
    getcwd(currDir, PATH_MAX);
    atleastOneDirVisited = true;
    printf("%s\n", currDir);
}

void warp(const char* token){
    getcwd(currDir, PATH_MAX); // Initialise it.
    if(token == NULL) changeDir(homeDirAbsPath);
    while(token != NULL){
        if(token[0] == '~') {
            char finalPath[8192] = {};
            strcat(finalPath, homeDirAbsPath);
            strcat(finalPath, token+1); // Ignore '~' and copy rest (if anything exists).
            changeDir(finalPath);
        }
        else if(token[0] == '-') {
            if(!atleastOneDirVisited) printf(BYEL "OLDPWD not set\n" CRESET);// This happens only for as long as the previous directory hasn't been set.
            else changeDir(prevDir);
        }
        else changeDir(token);
        token = strtok(NULL, delimiters);
    }
}