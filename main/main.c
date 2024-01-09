#include "headers.h"

// Will use main.c to handle input, to process commands. Can separate this into a different file, if necessary.

// To tokenize strings: strtok -> Reference: man strtok AND https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
// Whitespace characters in POSIX -> https://stackoverflow.com/questions/26597977/split-string-with-multiple-delimiters-using-strtok-in-c
// To remove trailing \n at end of fgets input: https://stackoverflow.com/questions/16677800/strtok-not-discarding-the-newline-character  AND man strchr - to locate first occurrence of a character in string.
// For colours in terminal: https://www.theurbanpenguin.com/4184-2/

// Signals: Normally, when child process stops, it sends SIGCHLD signal to the parent process. We can perhaps use this signal, to indicate the fact that a child process has stopped.
// However, to effectively process child background processes; I need to keep track of their names and process IDs.
// https://man7.org/linux/man-pages/man7/signal.7.html
// ADDENDUM: Q64 of the doubts document makes the above REDUNDANT.

// I/O redirection and piping: https://stackoverflow.com/questions/52939356/redirecting-i-o-in-a-custom-shell-program-written-in-c
// pipe() and dup2() or dup().
// https://man7.org/linux/man-pages/man2/pipe.2.html

// Checking if child process ended: https://stackoverflow.com/questions/60000733/how-to-check-if-all-child-processes-ended

// To detect Ctrl-D -> fgets returns NULL when it picks up an EOF and Ctrl-D puts an EOF into stdin -> https://stackoverflow.com/questions/29314106/fgets-should-stop-when-stdin-ends

bool isForegroundRunning = false;
char homeDirAbsPath[PATH_MAX] = {};
char delimiters[3] = " \t\n";

// Process multiple commands separated by ';' or '&' (SPEC 2)
int splitCmds(const char input[], char* allCmds[]){
    // Do not split the commands, if the commands are enclosed by quotes (either ' or ").
    int numCmds = 0, numChars = 0;

    for(int i = 0; input[i] != '\0'; i++){
        while(input[i] != '\0' && input[i] != ';' && input[i] != '&' && input[i] != '\"' && input[i] != '\'') i++, numChars++;
        if(input[i] == '\0' || input[i] == ';' || input[i] == '&'){
            allCmds[numCmds] = malloc((numChars+3)*sizeof(char));
            if(input[i] == '&') {
                strncpy(allCmds[numCmds], input + i - numChars, numChars);
                if(input[i-1] != ' ' && input[i-1] != '\t') allCmds[numCmds][numChars] = ' ', allCmds[numCmds][numChars+1] = '&';
                else allCmds[numCmds][numChars] = '&', allCmds[numCmds][numChars+1] = '\0';
                allCmds[numCmds][numChars+2] = '\0';
            }
            else if(input[i] == ';' || input[i] == '\0') {
                strncpy(allCmds[numCmds], input + i - numChars, numChars);
                allCmds[numCmds][numChars] = '\0';
            }
            numChars = 0, numCmds++;
        }
        else { // if input[i] is either " or ', search ahead until the closing " or ', and copy it over without splitting the & and ;.
            do i++, numChars++; // Need to do this once before.
            while(input[i] != '\0' && input[i] != '\"' && input[i] != '\'');
            numChars++; // To count the quote as an additional char.
            if(input[i+1] == '\0'){ // Only in the case where the quotes mark the end of the command.
                allCmds[numCmds] = malloc((numChars+3) * sizeof(char));
                strncpy(allCmds[numCmds], input + i + 1 - numChars, numChars + 1);
                allCmds[numCmds][numChars+1] = '\0';
                numCmds++;
            }
        }
    }

    return numCmds;
}

void runCmd(int* childPid, char* token1){
    isForegroundRunning = true; // For any of these except the ELSE, it is a foreground process.
    int saveSTDINMain = dup(STDIN_FILENO), saveSTDOUTMain = dup(STDOUT_FILENO);
    if(saveSTDOUTMain == -1){
        perror("Failed to save STDOUT");
        return;
    }
    if(saveSTDINMain == -1){
        perror("Failed to save STDIN");
        return;
    }

    // Process " " and ' ' separately:
    char* apostropheStart = strstr(token1, "\'");
    while(apostropheStart) {// Key: Map space to \a, tab to \f, > to \r, < to \v, | to \b. So, >> is \r\r.
        int endApostropheIdx = 1;
        for (int i = 1; apostropheStart[i] != '\0' && apostropheStart[i] != '\''; i++, endApostropheIdx++) {
            if (apostropheStart[i] == ' ') apostropheStart[i] = '\a';
            else if (apostropheStart[i] == '\t') apostropheStart[i] = '\f';
            else if (apostropheStart[i] == '>') apostropheStart[i] = '\r';
            else if(apostropheStart[i] == '<') apostropheStart[i] = '\v';
            else if(apostropheStart[i] == '|') apostropheStart[i] = '\b';
        }
        apostropheStart = strstr(apostropheStart+endApostropheIdx+1, "\'");
    }
    char* quotesStart = strstr(token1, "\"");
    while(quotesStart) {// Key: Map space to \a, tab to \f, > to \r, < to \v, | to \b. So, >> is \r\r.
        int endQuoteIdx = 1;
        for (int i = 1; quotesStart[i] != '\0' && quotesStart[i] != '\"'; i++, endQuoteIdx++) {
            if (quotesStart[i] == ' ') quotesStart[i] = '\a';
            else if (quotesStart[i] == '\t') quotesStart[i] = '\f';
            else if (quotesStart[i] == '>') quotesStart[i] = '\r';
            else if(quotesStart[i] == '<') quotesStart[i] = '\v';
            else if(quotesStart[i] == '|') quotesStart[i] = '\b';
        }
        quotesStart = strstr(quotesStart+endQuoteIdx+1, "\"");
    }

    // Handle invalid pipe cases:
    for(int i = 0; token1[i] != '\0'; i++) if(token1[i] != ' ' && token1[i] != '\t' && token1[i] != '\n') { // Check if a pipe occurs at the start.
        if(token1[i] == '|') {
            fprintf(stderr, BRED "ERROR: Pipe occurring at the start of command, with nothing to pipe input FROM!\n" CRESET);
            return;
        }
        break;
    }

    char* lastPipe = strrchr(token1, '|');
    if(lastPipe){ // Check if pipe occurs at the end.
        bool isValid = false;
        for(int i = 1; !isValid && lastPipe[i] != '\0'; i++) {
            if(lastPipe[i] != ' ' && lastPipe[i] != '\t' && lastPipe[i] != '\n') isValid = true; // If there exists a non-blank character after the last pipe, it is valid.
        }
        if(!isValid) {
            fprintf(stderr,
                    BRED "ERROR: Pipe occurring at the end of command, with nothing to pipe output TO!\n" CRESET); // This will execute if the above doesn't.
            return;
        }
    }

    // Handle cases where |||||| etc happen.
    char* findConsecPipes = strstr(token1, "||"); // |||| has || as substring, so, enough to find if || occurs as substring anywhere.
    if(findConsecPipes) {
        fprintf(stderr,
                BRED "ERROR: Multiple pipes put one after another in succession, with no commands in between.\n" CRESET);
        return;
    }

    // Handle pipes here:
    char* pipedCmds[MAX_PIPED_CMDS] = {NULL}, *tempTok = strtok(token1, "|");
    int numPipedCmds = 0;
    while(tempTok) pipedCmds[numPipedCmds++] = tempTok, tempTok = strtok(NULL, "|");
    int pipeFDs[numPipedCmds][2]; // n piped commands -> n-1 pipes. Have 1 extra to prevent "variable length array bound evaluates to non-positive value 0", but don't open a pipe there.
    for (int i = 0; i < numPipedCmds-1; i++) {
        if (pipe(pipeFDs[i]) < 0) {
            fprintf(stderr,
                    BRED "ERROR: Failed to open a pipe. Please ensure that the maximum number of file descriptors haven't been reached.\n" CRESET);
            return;
        }
    }

    for(int pip = 0; pip < numPipedCmds; pip++) { // Basically, take current output as input for next file.
        char* token = pipedCmds[pip];
        // Save STDIN and STDOUT file descriptors for us to be able to restore them later.
        int saveSTDIN = dup(STDIN_FILENO), saveSTDOUT = dup(STDOUT_FILENO);
        if(saveSTDOUT == -1){
            perror("Failed to save STDOUT");
            return;
        }
        if(saveSTDIN == -1){
            perror("Failed to save STDIN");
            return;
        }

        if(pip == 0 && numPipedCmds > 1){
            if(dup2(pipeFDs[pip][1], STDOUT_FILENO) < 0) {
                perror("Failed to duplicate STDOUT");
                return;
            }
            close(pipeFDs[pip][1]);
        }
        else if(pip < numPipedCmds-1){
            if(dup2(pipeFDs[pip][1], STDOUT_FILENO) < 0){
                perror("Failed to duplicate STDOUT");
                return;
            }
            if(dup2(pipeFDs[pip-1][0], STDIN_FILENO) < 0){
                perror("Failed to duplicate STDIN");
                return;
            }
            close(pipeFDs[pip-1][0]), close(pipeFDs[pip][1]);
        }
        else if(pip != 0 && pip == numPipedCmds-1){ // if last pipe
            if(dup2(pipeFDs[pip-1][0], STDIN_FILENO) < 0) {
                perror("Failed to duplicate STDIN");
                return;
            }
            close(pipeFDs[pip-1][0]);
        }

        // Handle piping and redirection here:
        bool redirectOutput = false, redirectInput = false, append = false; // KEY: append = false implies '>', append = true implies '>>'.
        char nameOfOutputFile[PATH_MAX] = {}, nameOfInputFile[PATH_MAX] = {}, newCmd[PATH_MAX] = {};
        char *newToken = NULL;

        // Check for input redirection:
        char *findInpRedir = strstr(token, "<");
        if (findInpRedir) {
            redirectInput = true;
            for (int i = 1; findInpRedir[i] != '\0'; i++)
                if (findInpRedir[i] != ' ' && findInpRedir[i] != '\t' &&
                    findInpRedir[i] != '\n') { // Find first non-empty char.
                    for (int j = i;
                         findInpRedir[j] != '\0' && findInpRedir[j] != ' ' && findInpRedir[j] != '>'; j++)
                        nameOfInputFile[j - i] = findInpRedir[j];
                    break;
                }
        }

        // Check for output redirection:
        char *findAppendRedir = strstr(token, ">>"), *findNonAppendRedir = strstr(token, ">");
        if (findAppendRedir || findNonAppendRedir) {
            redirectOutput = true;
            if (findAppendRedir) {
                append = true;
                for (int i = 2; findAppendRedir[i] != '\0'; i++)
                    if (findAppendRedir[i] != ' ' && findAppendRedir[i] != '\t' &&
                        findAppendRedir[i] != '\n') { // Find first non-empty char.
                        for (int j = i;
                             findAppendRedir[j] != '\0' && findAppendRedir[j] != ' ' && findAppendRedir[j] != '<'; j++)
                            nameOfOutputFile[j - i] = findAppendRedir[j];
                        break;
                    }
            } else {
                append = false;
                for (int i = 1; findNonAppendRedir[i] != '\0'; i++)
                    if (findNonAppendRedir[i] != ' ' && findNonAppendRedir[i] != '\t' &&
                        findNonAppendRedir[i] != '\n') { // Find first non-empty char.
                        for (int j = i; findNonAppendRedir[j] != '\0' && findNonAppendRedir[j] != ' ' &&
                                        findNonAppendRedir[j] != '<'; j++)
                            nameOfOutputFile[j - i] = findNonAppendRedir[j];
                        break;
                    }
            }
        }

        // Handle the change of file descriptors:
        if (redirectInput) {
            int inpFD = open(nameOfInputFile, O_RDONLY);
            if (inpFD < 0) {
                fprintf(stderr,
                        BRED "ERROR: Unable to redirect input from \"%s\".\n Please ensure that the file name is entered correctly, and that you have sufficient permissions to read from this file.\n" CRESET,
                        nameOfInputFile);
                break; // If such a file doesn't exist, then break out of this loop and restore original file descriptors for STDIN and STDOUT.
            }
            if(dup2(inpFD, STDIN_FILENO) < 0){ // Duplicate STDIN to the input file.
                perror("Failed to duplicate STDIN");
                return;
            }
            close(inpFD); // Close the input file descriptor as we no longer need it.
        }

        if (redirectOutput) {
            int outpFD;
            if (!append) {
                if (access(nameOfOutputFile, F_OK) != -1)
                    remove(nameOfOutputFile); // If file exists already, delete it.
                outpFD = open(nameOfOutputFile, O_CREAT | O_WRONLY, 0644);
            } else outpFD = open(nameOfOutputFile, O_CREAT | O_RDWR | O_APPEND, 0644);
            if (outpFD < 0) {
                fprintf(stderr,
                        BRED "ERROR: Unable to redirect output from \"%s\".\nPlease ensure that you have sufficient permissions to access this file and its parent directory.\n" CRESET,
                        nameOfOutputFile);
            }
            if(dup2(outpFD, STDOUT_FILENO) < 0){ // Duplicate STDOUT to the output file.
                perror("Failed to duplicate STDOUT");
                return;
            }
            close(outpFD); // Close the output file descriptor as we no longer need it.
        }

        // Now, copy over everything except the > and < and subsequent info, to a new token so that we can use it to run command.
        for (int i = 0; token[i] != '\0' && token[i] != '>' && token[i] != '<'; i++) newCmd[i] = token[i];
        newToken = strtok(newCmd, delimiters);

        // Handle actual command execution here:
        if (strcmp(newToken, "warp") == 0) newToken = strtok(NULL, delimiters), warp(newToken);
        else if (strcmp(newToken, "peek") == 0) newToken = strtok(NULL, delimiters), peekHelper(newToken);
        else if (strcmp(newToken, "pastevents") == 0) newToken = strtok(NULL, delimiters), pastevents_handler(childPid, newToken);
        else if (strcmp(newToken, "proclore") == 0) newToken = strtok(NULL, delimiters), proclore(newToken);
        else if (strcmp(newToken, "seek") == 0) newToken = strtok(NULL, delimiters), seekHandler(newToken);
        else if (strcmp(newToken, "activities") == 0) activities();
        else if (strcmp(newToken, "ping") == 0) newToken = strtok(NULL, delimiters), ping(newToken);
        else if (strcmp(newToken, "fg") == 0) newToken = strtok(NULL, delimiters), fg(newToken);
        else if (strcmp(newToken, "bg") == 0) newToken = strtok(NULL, delimiters), bg(newToken);
        else if (strcmp(newToken, "neonate") == 0) newToken = strtok(NULL, delimiters), neonate(newToken);
        else if (strcmp(newToken, "iMan") == 0) newToken = strtok(NULL, delimiters), iMan(newToken);
        else if (strcmp(newToken, "exit") == 0) ctrlDInterrupt(); // Basically, do exactly what Ctrl-D does when "exit" is typed into the terminal.
        else { // Either it is a system command, or it is invalid.
            char *checkForeground = strstr(newToken, "&");
            if (!checkForeground) isForegroundRunning = false;
            *childPid = processSysCmd(newToken); // Assume that it is a sys command and proceed, if it is not a valid custom-defined command.
            if (*childPid == -2)
                fprintf(stderr, BRED "ERROR: fork() reported a failure! Please ensure that your system has the necessary resources to facilitate forking, and that there aren't too many child processes running concurrently!\n" CRESET);
        }

        // Restore I/O redirection or piping STDIN and STDOUT.
        if(dup2(saveSTDIN, STDIN_FILENO) < 0){
            perror("Failed to restore STDIN");
            return;
        }
        if(dup2(saveSTDOUT, STDOUT_FILENO) < 0){
            perror("Failed to restore STDOUT");
            return;
        }
        close(saveSTDIN), close(saveSTDOUT);
    }
    // Restore main STDIN and STDOUT.
    if(dup2(saveSTDINMain, STDIN_FILENO) < 0){
        perror("Failed to restore STDIN");
        return;
    }
    if(dup2(saveSTDOUTMain, STDOUT_FILENO) < 0){
        perror("Failed to restore STDOUT");
        return;
    }
    close(saveSTDINMain), close(saveSTDOUTMain);
}

int main()
{
    // Store path of home directory to process in prompt().
    getcwd(homeDirAbsPath, PATH_MAX);
    strcpy(prevDir, homeDirAbsPath); // Initialise this, for warp and other commands which need the previous directory.

//    signal(SIGCHLD, processBGSIGCHLD); // Process a child that terminates (for background processes) -> REDUNDANT, refer to Q64 of the doubts document.
    signal(SIGINT, ctrlCorZInterrupt); // To handle CTRL-C, this has been handled in signals.
    signal(SIGTSTP, ctrlCorZInterrupt); // To handle CTRL-Z, this has been handled in signals.

    while (1) {
        isForegroundRunning = false;

        usleep(50000); // 50 ms of sleep time, so that perror has enough time to print before next prompt.
        static time_t totalTime = -1; // Init this only once.
        static char strForNextPrompt[4096] = {}; // Init this only once.

        prompt(strForNextPrompt, totalTime);
        char input[4096] = {};
        int childPid = 0; // ONLY FOR THE processSysCmd. IF IT IS -2, THAT MEANS FORKING FAILED.
        if(fgets(input, 4096, stdin) == NULL) ctrlDInterrupt(); // Note: This has been handled in bgHandler, not signals!

        while(processBGSIGCHLD() > 0); // Will collect ALL ended child processes, whenever we type another command or type enter, before the next prompt is generated.

        // Check if input is actually valid, i.e., there's a non-blank character.
        bool isValid = false;
        for(int i = 0; !isValid && input[i] != '\0'; i++) if(input[i] != ' ' && input[i] != '\t' && input[i] != '\n') isValid = true;
        if(!isValid) continue;

        // Store commands in pastevents. Let it handle pastevents execute, and other pastevents variants, itself.
        char* tempCpy =  malloc(4096*sizeof(char)); // Need to do this, else, pastevents_add will tokenise and ruin the string.
        strcpy(tempCpy, input);

        // Remove trailing newline escape character:
        char* newLineEsc = strchr(input, '\n'); // Searches for first occurrence of newline char and if found, returns pointer to it. Else, returns NULL.
        if (newLineEsc) *newLineEsc = '\0';

        // Store given input for next prompt, if total time taken for command execution is > 2 seconds.
        strcpy(strForNextPrompt, input);

        char* allCmds[512] = {NULL};
        int numCmds = splitCmds(input, allCmds);

        time_t startTime = time(NULL); // To track start time.
        for(int i = 0; i < numCmds; i++) {
            runCmd(&childPid, allCmds[i]);
            free(allCmds[i]), allCmds[i] = NULL;
        }
        totalTime = time(NULL)-startTime; // To track end time.

        pastevents_add(tempCpy); // Store the given input (as it is) in history.
        free(tempCpy), tempCpy = NULL;
    }
}
