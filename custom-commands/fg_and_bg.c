#include "../main/headers.h"

// Note: The foreground process handler code is the same as that in syscmds, with a small change wherein I add the SIGCONT signal.

char* getCmdName(pid_t pid){ // Check if given PID exists, handle errors if it doesn't, and obtain it's cmdName if it does.
    char tempBuf[PATH_MAX] = {}, cmdName[BUFSIZ] = {};
    sprintf(tempBuf, "/proc/%d/stat", (int)pid);
    FILE* fPtr = fopen(tempBuf, "r");
    if(!fPtr) { // Do this only for pid > 0, else it is a security issue where you will be allowed to ping all running processes.
        fprintf(stderr, BRED "ERROR: No such process found!\n" CRESET);
        return NULL;
    }
    fscanf(fPtr, "%*s %s", cmdName);
    int strLen = strlen(cmdName);
    char* cmdNameFinal = (char*)malloc((strLen-1)*sizeof(char));
    for(int i = 1; i < strLen - 1; i++) cmdNameFinal[i-1] = cmdName[i]; // To remove the parentheses ( ) around the command name.
    cmdNameFinal[strLen-2] = '\0';
    fclose(fPtr);
    return cmdNameFinal;
}

void fg(char* token){
    char* endPtr; // Just for strtol's error checking.
    long pid;
    if(!token) {
        fprintf(stderr, BRED "ERROR: Missing arguments!\nCommand usage: fg <pid>\n" CRESET);
        return;
    }
    else{
        pid = strtol(token, &endPtr, 10);
        if(endPtr == token || pid > INT_MAX) {
            fprintf(stderr, BRED "ERROR: Invalid process ID! Please input a valid 32-bit integer for pid.\nCommand usage: fg <pid>\n" CRESET);
            return;
        }
    }
    bool isInList = isProcessInList((int)pid);

    char* cmdName = getCmdName((int)pid);
    if(!cmdName) return; // Given PID doesn't exist. Error handling is done by the getCmdName function itself.

    setpgid((int)pid, getpgid(0)); // Set child's group ID to that of the controlling terminal's group ID (this is true for FG processes).
    if(!isInList) addBackgroundProcess(cmdName, (int)pid), isInList = true; // Add it, considering the possibility of it turning into a background process in the future.
    free(cmdName);

    int status; // status is var to store the status of child process determined by waitpid() in parent.
    signal(SIGTTIN, SIG_IGN), signal(SIGTTOU, SIG_IGN); // While child process is active, ignore the SIGTTIN and SIGTTOU signals sent to the child process if it tries to read from/write to the terminal (do not terminate the process).
    tcsetpgrp(0, (int)pid); // Give the child process full access to the terminal, while it is alive. Child pgrp is the same as childpid initially.

    if (kill((int)pid, SIGCONT) < 0) { // Send a signal to continue the process, i.e., change its state from stopped to running.
        perror("Failed to resume the process");
        removeProcess((int)pid);
        isInList = false;
    }
    // Even if the above command fails, proceed, since we do not want this process to turn into a zombie. The rest of the code will handle it.

    waitpid((int)pid, &status, WUNTRACED); // Wait for child process to finish executing. WUNTRACED flag -> return immediately if no child has executed (functionality of WNOHANG), but also return if a child has stopped.
    if(isInList && WEXITSTATUS(status) != 20) removeProcess((int)pid); // Remove the fg process from the list, unless when it becomes a background process due to SIGTSTP (Ctrl-Z).

    tcsetpgrp(0, getpgid(0)); // Make the parent terminal reclaim control of the terminal, once the child finishes executing.
    signal(SIGTTIN, SIG_DFL), signal(SIGTTOU, SIG_DFL); // After the child process stops, let the parent process resume default control of SIGTTIN and SIGTTOU signals.

    if(WEXITSTATUS(status) == EXIT_FAILURE) fprintf(stderr, BRED "ERROR: An error has occurred while attempting to bring this process to foreground, as the given pid corresponds to an invalid process.\n" CRESET); // Let main.c know that the execvp failed, probably because such a command doesn't exist
}

void bg(char* token){
    char* endPtr; // Just for strtol's error checking.
    long pid;
    if(!token) {
        fprintf(stderr, BRED "ERROR: Missing arguments!\nCommand usage: bg <pid>\n" CRESET);
        return;
    }
    else{
        pid = strtol(token, &endPtr, 10);
        if(endPtr == token) {
            fprintf(stderr, BRED "ERROR: Invalid process ID! Please input a valid 32-bit integer for pid.\nCommand usage: bg <pid>\n" CRESET);
            return;
        }
    }
    bool isInList = isProcessInList((int)pid);

    char* cmdName = getCmdName((int)pid);
    if(!cmdName) return; // Given PID doesn't exist. Error handling is done by the getCmdName function itself.
    if(!isInList) addBackgroundProcess(cmdName, (int)pid); // Add it, since it is a background process.
    free(cmdName);
    if (kill((int)pid, SIGCONT) < 0) {
        perror("Failed to resume the process"); // Send a signal to continue the process, i.e., change its state from stopped to running.
        removeProcess((int)pid);
    }
}