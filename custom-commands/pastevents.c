#include "../main/headers.h"

// Working: To add a new command to history, check if there already exist 15 commands. If no, then add it. To do this, do some file processing (read all the lines into an array, and remove
// the topmost (first) line. Add new line at end, and write lines 2 to 16 to your history file.

// UPDATE TO PASTEVENTS: TO ADD SUPPORT FOR pastevents execute, if it is found in a string, I shall immediately replace it with the corresponding command.

// To read line-by-line -> getline -> https://c-for-dummies.com/blog/?p=1112

void pastevents_handler(int* childPid, char* token){
    if(token == NULL) pastevents_print();
    else if(strcmp(token, "execute") == 0) {
        token = strtok(NULL, delimiters);
        if(!token) fprintf(stderr, BRED "ERROR: Missing arguments for pastevents execute! Please enter a valid argument.\n" CRESET);
        char* execCmd = pastevents_execute(token);
        if(!execCmd) return;

        // Remove trailing newline escape character:
        char* newLineEsc = strchr(execCmd, '\n'); // Searches for first occurrence of newline char and if found, returns pointer to it. Else, returns NULL.
        if (newLineEsc) *newLineEsc = '\0';

        char* allCmds[512] = {NULL};
        int numCmds = splitCmds(execCmd, allCmds);

        for(int i = 0; i < numCmds; i++) {
            runCmd(childPid, allCmds[i]);
            free(allCmds[i]);
        }

        free(execCmd);
    }
    else if(strcmp(token, "purge") == 0) pastevents_purge();
    else fprintf(stderr, BRED "ERROR: Invalid argument for pastevents! Please enter a valid argument.\n" CRESET);
}

int pastevents_retrieve(char* allCmds[MAX_FILES_IN_HISTORY+1]){ // Useful for "pastevents" as well.
    for(int i = 0; i < MAX_FILES_IN_HISTORY+1; i++) allCmds[i] = NULL;
    char path[2*PATH_MAX] = {};
    int count = 0;
    unsigned long maxSize = 4097; // 4096 + 1 for '\0'
    sprintf(path, "%s/.history.txt", homeDirAbsPath);

    FILE* historyRead = fopen(path, "r");
    if(historyRead == NULL) { // Create the file if it doesn't exist.
        int FD = open(path, O_CREAT, 0644);
        close(FD);
        historyRead = fopen(path, "r");
    }
    while(getline(&allCmds[count], &maxSize, historyRead) > 0) {count++;}
    fclose(historyRead);

    return count;
}

void pastevents_add(char* input){
    // Check for validity of input first (don't store just \n, \0 or empty inputs):
    bool isValid = false;
    for(int i = 0; !isValid && input[i] != '\0'; i++) if(input[i] != ' ' && input[i] != '\t' && input[i] != '\n') isValid = true;
    if(!isValid) return;

    char path[2*PATH_MAX] = {};
    sprintf(path, "%s/.history.txt", homeDirAbsPath);
    char* allCmds[MAX_FILES_IN_HISTORY+1] = {NULL};
    int numCmds = pastevents_retrieve(allCmds);

    // Check if last command entered is given command:
    if(numCmds > 0 && strcmp(allCmds[numCmds-1], input) == 0) return;

    // Now, write the new command along with all previous commands (unless there are 15 already, in which case remove the first.)
    FILE* historyAdd = fopen(path, "w");
    if(historyAdd == NULL) fprintf(stderr, BRED "ERROR: Unable to open/create the .history.txt file! Please ensure that you have sufficient permissions to run this shell!\n" CRESET);

    // Process commands containing the word "pastevents".
    char newStr[4096] = {}, referenceStr[4096] = {};
    strcpy(referenceStr, input);
    bool storeNewStr = true, atleastOneModif = false;
    int prevIdx = 0, newStrIdx = 0;
    char* whenPasteventsStarts = strstr(input, "pastevents");
    if(whenPasteventsStarts) {
        char *tempToken = strtok(whenPasteventsStarts, delimiters);
        while (whenPasteventsStarts) {
            bool placeSemiColon = true;
            tempToken = strtok(NULL, delimiters);
            if (tempToken && strcmp(tempToken, "execute") == 0) {
                atleastOneModif = true;
                tempToken = strtok(NULL, delimiters);
                if(!tempToken) return;
                int pasteventsExecIdx = atoi(tempToken);
                long idx = whenPasteventsStarts - input;
                for (int i = prevIdx; i < idx; i++) {
                    newStr[newStrIdx++] = referenceStr[i];
                }
                if (!(pasteventsExecIdx == 0 || pasteventsExecIdx > numCmds)) {
                    int len = strlen(allCmds[numCmds - pasteventsExecIdx]);
                    for (int i = newStrIdx; i < newStrIdx + len - 1; i++) {
                        newStr[i] = allCmds[numCmds - pasteventsExecIdx][i - newStrIdx];
                        if(allCmds[numCmds - pasteventsExecIdx][i - newStrIdx] == '&') placeSemiColon = false;
                    }
                    newStrIdx += len - 1;
                }
                prevIdx = (tempToken + strlen(tempToken)) - input;
                int tempIdxLookahead = prevIdx;
                while((referenceStr[tempIdxLookahead] == ' ' || referenceStr[tempIdxLookahead] == '\t')) tempIdxLookahead++;
                if(referenceStr[tempIdxLookahead] == ';' || referenceStr[tempIdxLookahead] == '\0' || referenceStr[tempIdxLookahead] == '\n' || referenceStr[tempIdxLookahead] == '|' || referenceStr[tempIdxLookahead] == '>' || referenceStr[tempIdxLookahead] == '<') placeSemiColon = false;
                if(placeSemiColon) newStr[newStrIdx++] = ';';
                while (tempToken && strcmp(tempToken, "pastevents") != 0)
                    tempToken = strtok(NULL, delimiters); // Search for next occurrence.
                whenPasteventsStarts = tempToken;
            } else { // Else, do nothing; don't modify the history file as we won't be storing a command with just "pastevents" or "pastevents purge" anyways.
                storeNewStr = false;
                break;
            }
        }
    }

    int i = 0;
    if(storeNewStr && numCmds == 15) i = 1;
    while(i < numCmds) fprintf(historyAdd, "%s", allCmds[i++]);
    if(storeNewStr && atleastOneModif) {
        int finalIdx = strlen(referenceStr);
        for(int j = prevIdx; j < finalIdx; j++){
            newStr[newStrIdx++] = referenceStr[j];
        }
        newStr[newStrIdx] = '\0';
        if(numCmds > 0 && strcmp(allCmds[numCmds-1], newStr) != 0 && newStr[0] != '\n' && newStr[0] != '\0') fprintf(historyAdd, "%s", newStr);
    }
    else if(storeNewStr) fprintf(historyAdd, "%s", referenceStr);

    for(i = 0; i <= numCmds; i++) free(allCmds[i]), allCmds[i] = NULL;

    fclose(historyAdd);
}

void pastevents_print(){
    char* allCmds[MAX_FILES_IN_HISTORY+1] = {NULL};
    int numCmds = pastevents_retrieve(allCmds);
    for(int i = 0; i < numCmds; i++) printf("%s", allCmds[i]), free(allCmds[i]), allCmds[i] = NULL;
    free(allCmds[numCmds]), allCmds[numCmds] = NULL;
}

char* pastevents_execute(char* token){
    if(!token) return NULL;
    int idx = atoi(token);
    char* allCmds[MAX_FILES_IN_HISTORY+1] = {NULL};
    int numCmds = pastevents_retrieve(allCmds);
    if(numCmds == 0){
        fprintf(stderr, BRED "ERROR: Invalid index! pastevents is currently empty.\n" CRESET);
        return NULL;
    }
    if(idx == 0 || idx > numCmds) {
        fprintf(stderr, BRED "ERROR: Invalid index! Please enter a valid integer between 1 (latest) and %d (earliest of the past 15).\n" CRESET, numCmds);
        return NULL;
    }

    char* reqCmd = malloc((strlen(allCmds[numCmds-idx])+1) * sizeof(char));
    strcpy(reqCmd, allCmds[numCmds-idx]);
    for(int i = 0; i <= numCmds; i++) free(allCmds[i]), allCmds[i] = NULL;
    return reqCmd;
}

void pastevents_purge(){ // Just open and close the history file to purge its contents.
    char path[2*PATH_MAX] = {};
    sprintf(path, "%s/.history.txt", homeDirAbsPath);

    remove(path);
    int FD = open(path, O_CREAT, 0644);
    close(FD);
}