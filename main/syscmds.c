#include "headers.h"

// Reference: For foreground processes, to wait for them -> use waitpid() -> https://linux.die.net/man/2/waitpid AND man waitpid
// Actual process execution -> https://linux.die.net/man/3/execvp

// How to give a child access to the terminal while it is alive -> https://stackoverflow.com/questions/20716967/provide-full-access-over-terminal-for-child-while-it-alive
// tcgetpgrp() -> https://linux.die.net/man/3/tcsetpgrp AND https://www.ibm.com/docs/en/zos/2.3.0?topic=functions-tcsetpgrp-set-foreground-process-group-id
// Use of tcgetpgrp() -> Give the child process full access to the terminal, while it is alive, and also reclaim control after it is done executing (must be used twice).
// Arg1 -> fd -> pid of controlling terminal. 0 in most cases. Arg2 -> pgrp -> nonempty process group belonging to same session as the terminal which calls the process.
// setpgid() -> https://man7.org/linux/man-pages/man2/setpgid.2.html -> Used in conjunction with tcgetpgrp(). Set the child process's pgrp to that of the controlling terminal, so that access can be given to it.
// NOTE for setpgid() -> If pid is zero, then the process ID of the calling process is used.  If pgid is zero, then the PGID of the process specified by pid is made the same as its process ID.

// Interactive shell text editors -> https://www.reddit.com/r/C_Programming/comments/ek2mw6/how_can_i_call_vim_and_other_programs_using_execve/

// SIGNALS:
// Main parent shell kept stopping after child process -> Had to use signals -> https://man7.org/linux/man-pages/man7/signal.7.html
// Had to read up on: https://www.gnu.org/software/libc/manual/html_node/Job-Control-Signals.html
// and also https://en.cppreference.com/w/c/program/SIG_strategies

int bringProcessToFG(char* cmdName, int childPID){
    addBackgroundProcess(cmdName, childPID); // Add it, considering the possibility of it turning into a background process in the future.

    int status; // status is var to store the status of child process determined by waitpid() in parent.
    signal(SIGTTIN, SIG_IGN), signal(SIGTTOU, SIG_IGN); // While child process is active, ignore the SIGTTIN and SIGTTOU signals sent to the child process if it tries to read from/write to the terminal (do not terminate the process).
    tcsetpgrp(0, childPID); // Give the child process full access to the terminal, while it is alive. Child pgrp is the same as childpid initially.

    waitpid(childPID, &status, WUNTRACED); // Wait for child process to finish executing. WUNTRACED flag -> return immediately if no child has executed (functionality of WNOHANG), but also return if a child has stopped.

    if(WEXITSTATUS(status) != 20) removeProcess(childPID); // Remove the fg process from the list, unless when it becomes a background process due to SIGTSTP (Ctrl-Z).

    tcsetpgrp(0, getpgid(0)); // Make the parent terminal reclaim control of the terminal, once the child finishes executing.
    signal(SIGTTIN, SIG_DFL), signal(SIGTTOU, SIG_DFL); // After the child process stops, let the parent process resume default control of SIGTTIN and SIGTTOU signals.

    if(WEXITSTATUS(status) == EXIT_FAILURE) return -1; // Let main.c know that the execvp failed, probably because such a command doesn't exist.
    else if(WIFSTOPPED(status)) return 0; // Child process has stopped successfully. Let the parent terminal know.
    return 0;
}

int processSysCmd(char* token){
    // Read all arguments first to decide if it is a background or foreground process.
    bool background = false;
    char* allArgs[MAX_ARGS] = {NULL}; // Worst case scenario; optimistically assuming there won't be more than MAX_ARGS args for any command.
    int i = 0; // i is itr variable
    char* cmdName = token;
    while(token) {
        for(int j = 0; token[j] != '\0'; j++) {
            if(token[j] == '\a') token[j] = ' ';
            else if(token[j] == '\f') token[j] = '\t';
            else if(token[j] == '\r') token[j] = '>';
            else if(token[j] == '\v') token[j] = '<';
            else if(token[j] == '\b') token[j] = '|';
            else if(token[j] == '\"' || token[j] == '\''){ // Pop that char.
                for(int k = j; token[k] != '\0'; k++) token[k] = token[k+1];
                j--; // to stay at same value of j for next iteration.
            }
        }
        allArgs[i++] = token;
        token = strtok(NULL, delimiters);
    }
    if(strcmp(allArgs[i-1], "&") == 0) background = true, allArgs[i-1] = NULL;
    int childPID = fork();
    if(childPID < 0) { // Error while forking
        perror("Failed while attempting to fork parent process");
        return -2;
    }
    else if(childPID > 0){ // Parent process
        if(background){ // Then we must print child PID.
            addBackgroundProcess(cmdName, childPID);
            printf("%d\n", childPID);
            return childPID; // Only background processes return a value > 0 and have a totalTime of -1.
        }
        else { // Foreground process; so parent must wait for child to finish executing.
            return bringProcessToFG(cmdName, childPID);
        }
    }
    else { // Child process
        setpgid(0, 0); // This way, if we give it its own process ID, then it will get properly detected as a background process, if applicable. (For foreground processes, we use tcsetpgrp in the "else" above.)
        execvp(allArgs[0], allArgs); // Only if execvp returns some value, i.e., the execution fails, the following commands execute. Else, child process gets taken over by execvp immediately.

        // FOLLOWING COMMANDS EXECUTE ONLY WHEN returnVal RETURNS A NEGATIVE VALUE, I.E., EXECUTION (execvp) FAILS!
        if(errno == ENOENT) fprintf(stderr, BRED "ERROR: '%s' is not a valid command\n" CRESET, allArgs[0]);
        if(errno != 2) perror("Failed to execute the process"); // Print every other error other than ENOENT.
        exit(EXIT_FAILURE); // Make the child process exit with an error.
    }
}