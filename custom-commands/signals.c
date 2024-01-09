#include "../main/headers.h"

// References: kill() -> https://www.ibm.com/docs/en/zos/2.4.0?topic=functions-kill-send-signal-process AND man kill
// List of signals -> https://man7.org/linux/man-pages/man7/signal.7.html
// strtol() advantage over atoi() -> https://stackoverflow.com/questions/8871711/atoi-how-to-identify-the-difference-between-zero-and-error

// CTRL-C -> Kill only the foreground process, if one is currently running. Note that SIGINT will kill the current running foreground process anyway, this is just to ensure that the shell won't be killed.
// CTRL-Z -> Push any foreground process to the background and change its state from "Running" to "Stopped." NOTE THAT SIGTSTP DOES THIS BY ITSELF, this is just to ensure that the shell itself doesn't become a background process.
void ctrlCorZInterrupt(){
    // This is necessary to ensure that you do not accidentally kill/background the shell itself!
    if(!isForegroundRunning) printf(BYEL "No foreground process is currently running. Nothing has been killed or sent to background!\nDo note that you can still type your command into the prompt, and it will get executed.\n" CRESET);
    else isForegroundRunning = false;
    fflush(stdin), fflush(stdout);
}

void ping(char* token){
    char* endPtr; // for strtol
    long pid = -1, signal_number = -1, retVal = 0;

    // Process input:
    if(!token) {
        fprintf(stderr, BRED "ERROR: Missing arguments!\nCommand usage: ping <pid> <signal_number>\n" CRESET);
        return;
    }
    else{
        pid = strtol(token, &endPtr, 10);
        if(endPtr == token || pid > INT_MAX) {
            fprintf(stderr, BRED "ERROR: Invalid process ID! Please input a valid 32-bit integer for pid.\nCommand usage: ping <pid> <signal_number>\n" CRESET);
            return;
        }
        token = strtok(NULL, delimiters);
    }
    if(!token){
        fprintf(stderr, BRED "ERROR: Missing arguments!\nCommand usage: ping <pid> <signal_number>\n" CRESET);
        return;
    }
    else{
        signal_number = strtol(token, &endPtr, 10);
        if(endPtr == token) {
            fprintf(stderr, BRED "ERROR: Invalid signal number! Please input a valid signal number (an integer).\nCommand usage: ping <pid> <signal_number>\n" CRESET);
            return;
        }
        signal_number = ((signal_number % 32) + 32) % 32;
    }

    retVal = kill((pid_t)pid, (int)signal_number);
    if(retVal < 0) {
        perror("Unable to send a signal to this process");
        return;
    }
    printf(BGRN "Sent signal %ld to process with pid %ld.\n" CRESET, signal_number, pid);

}