#include "headers.h"

// NOTE: For activities -> We know that only background processes can exist, which might possibly be running that are spawned by the shell.
// Any other process, if foreground, would've exited before we are able to access the shell again. So, for this reason, I need only print the background processes.

// Signals: Normally, when child process stops, it sends SIGCHLD signal to the parent process. We can perhaps use this signal, to indicate the fact that a child process has stopped.
// However, to effectively process child background processes; I need to keep track of their names and process IDs.
// https://man7.org/linux/man-pages/man7/signal.7.html
// https://linux.die.net/man/2/waitpid -> waitpid -1 means wait for any child process.

// To check if a specific process is stopped or running -> https://unix.stackexchange.com/questions/491764/is-it-possible-to-check-if-a-specific-process-is-sleeping-or-running AND man stat -> status code is T if it is stopped.

typedef struct processInfo{
    pid_t pid;
    char* cmd;
} procInfo;

int status; // Status of child process (only for background process handler -> processBGSIGCHLD
int currNumBGProcesses = 0;
procInfo* BGprocesses[MAX_CONCURRENT_BG_PROCESSES] = {NULL};

int comparator(const void* A, const void* B){
    procInfo* a = *((procInfo**) A);
    procInfo* b = *((procInfo**) B);
    if(!a) return 1;
    else if(!b) return -1;
    return strcmp(a->cmd, b->cmd);
}

void addBackgroundProcess(char* token, int childPid){
    procInfo* process = (procInfo*)malloc(sizeof(procInfo));
    process->pid = childPid;
    char* cmdCpy = malloc((2*strlen(token) + 1)*sizeof(char));
    strcpy(cmdCpy, token);
    process->cmd = cmdCpy;
    for(int i = 0; i < MAX_CONCURRENT_BG_PROCESSES; i++) if(!BGprocesses[i]) {BGprocesses[i] = process; currNumBGProcesses++; break;}
    qsort(BGprocesses, currNumBGProcesses, sizeof(procInfo*), comparator);
}

void removeProcess(int childPid){
    for(int i = 0; i < currNumBGProcesses; i++){
        if(BGprocesses[i] && BGprocesses[i]->pid == childPid){
            free(BGprocesses[i]->cmd), free(BGprocesses[i]);
            BGprocesses[i] = NULL;
            qsort(BGprocesses, currNumBGProcesses, sizeof(procInfo*), comparator); // To sort the NULL to the end of currently active processes.
            currNumBGProcesses--;
            break;
        }
    }
}

bool isProcessInList(pid_t pid){
    for(int i = 0; i < currNumBGProcesses; i++) if(BGprocesses[i]->pid == pid) return true;
    return false;
}

int processBGSIGCHLD(){
    int childPid = waitpid(-1, &status, WNOHANG); // I tried WUNTRACED first, but it didn't work (after bg process, if you ran fg process, it kept waiting for ALL bg processes to end first lol.). WNOHANG worked like a charm.
    if(childPid > 0){
        for(int i = 0; i < currNumBGProcesses; i++){
            if(BGprocesses[i] && BGprocesses[i]->pid == childPid){
                bool isNormalExit = WIFEXITED(status);
                if(isNormalExit) printf(BGRN "%s exited normally (%d)\n" CRESET, BGprocesses[i]->cmd, BGprocesses[i]->pid);
                else fprintf(stderr, BRED "ERROR: %s exited abnormally (%d)\n" CRESET, BGprocesses[i]->cmd, BGprocesses[i]->pid);
                free(BGprocesses[i]->cmd), free(BGprocesses[i]);
                BGprocesses[i] = NULL;
                qsort(BGprocesses, currNumBGProcesses, sizeof(procInfo*), comparator); // To sort the NULL to the end of currently active processes.
                currNumBGProcesses--;
                break;
            }
        }
    }
    return childPid;
}

void activities(){
    if(currNumBGProcesses == 0) printf(BYEL "No processes are currently active!\n" CRESET);
    for(int i = 0; i < currNumBGProcesses; i++){
        if(!BGprocesses[i]) continue; // This should never trigger (since processBGSIGCHILD's qsort should auto push the NULLs to the end), but just in case.
        char statusTemp;
        char tempBuffer[PATH_MAX];
        sprintf(tempBuffer, "/proc/%d/stat", BGprocesses[i]->pid);
        FILE* filePtr = fopen(tempBuffer, "r");
        if(!filePtr) {
            fprintf(stderr, BRED "ERROR: A process with this process ID doesn't exist. Please ensure that the entered pid is valid!\n" CRESET);
            return;
        }
        fscanf(filePtr, "%*d %*s %c", &statusTemp); // * before format specifier will not store that value. Basically, ignore that value.
        fclose(filePtr);
        printf(BCYN "%d" CRESET ":" MAG " %s" CRESET " - ", BGprocesses[i]->pid, BGprocesses[i]->cmd);
        if(statusTemp != 'T') printf(BGRN "Running\n" CRESET); // Any state that is NOT stopped ("T" in state in /proc) is considered to be running.
        else printf(BRED "Stopped\n" CRESET);
    }
}

void ctrlDInterrupt(){ // Kill all background processes spawned by this shell, and kill the shell itself.
    int retVal = 0;
    for(int i = 0; i < currNumBGProcesses; i++){
        if(!BGprocesses[i]) continue; // This should never trigger (since processBGSIGCHILD's qsort should auto push the NULLs to the end), but just in case.
        retVal = kill(BGprocesses[i]->pid, SIGKILL);
        if(retVal < 0) {
            perror("ERROR: Unable to send a signal to a process");
            fprintf(stderr, BRED "Process ID is %d.\n" CRESET, BGprocesses[i]->pid);
            return;
        }
        printf(BGRN "Killed process with pid %d.\n" CRESET, BGprocesses[i]->pid);
        free(BGprocesses[i]->cmd), free(BGprocesses[i]);
        BGprocesses[i] = NULL;
    }
    currNumBGProcesses = 0;
    fflush(stdin); // To remove that EOF.
    printf(BRED "Logging out of shell!\n" CRESET);
    exit(0); // Exit the shell.
}
