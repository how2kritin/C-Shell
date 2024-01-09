#include "../main/headers.h"

// References:
// Starter code from: https://gist.github.com/schlechter-afk/e4f6df2868ed0ba1d780747535c54d4e

// OLD (REDUNDANT) WAY:
// It is basically https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html -> USING READ() TIMEOUT TO JUST PROCEED AFTER 1ms IF NO INPUT IS GIVEN.
// NOTE: when read() times out, it returns 0.

// Better way:
// I just fork, child will keep printing and parent alone will be in raw mode, and it will keep looking for input of 1 byte at least, without timing out.
// As soon as the parent receives that input, it will kill the child.

// How to get last created process ID?
// OLD (REDUNDANT) WAY:
// Since /proc stores info on all files, and process IDs are increasing in number, I can find the maximum process ID number in that directory perhaps?
// https://stackoverflow.com/questions/63372288/getting-list-of-pids-from-proc-in-linux
// http://www.qnx.com/developers/docs/qnxcar2/index.jsp?topic=%2Fcom.qnx.doc.neutrino.cookbook%2Ftopic%2Fs3_procfs_Iterating_through_processes.html

// BETTER WAY I FIGURED OUT FROM https://man7.org/linux/man-pages/man5/proc.5.html (had to do some reading of this man page though).
// USE /proc/sys/kernel/ns_last_pid to get the last PID assigned by the kernel.


// I'm using sleep() to print something every X seconds -> man sleep.

int max(int a, int b){
    return a > b ? a : b;
}

// Old way:
//int latestPID(){
//    struct dirent *dirent;
//    DIR *dir;
//    int maxPID = -1;
//
//    if(!(dir = opendir("/proc"))) {
//        perror("ERROR: Unable to open \"/proc\" directory");
//        return -1;
//    }
//
//    dirent = readdir(dir);
//    while(dirent){
//        if(isdigit(*dirent->d_name)) maxPID = max(maxPID, atoi(dirent->d_name)); // Check if first char of file name is a digit. Not a foolproof method, but will help atoi nonetheless, as we do not need to do atoi on every single filename.
//        dirent = readdir(dir);
//    }
//
//    closedir(dir);
//    return maxPID;
//}

// Better way: https://man7.org/linux/man-pages/man5/proc.5.html
int latestPID(){
    FILE* ns_last_pid = fopen("/proc/sys/kernel/ns_last_pid", "r");
    if(ns_last_pid){
        int lastPID;
        fscanf(ns_last_pid, "%d", &lastPID);
        fclose(ns_last_pid);
        return lastPID;
    }
    fprintf(stderr, BRED "Failed to open the \"/proc/sys/kernel/ns_last_pid\" file. Kindly ensure that you have sufficient privileges to do so.\n" CRESET);
    return -1;
}

void die(const char *s) {
    perror(s);
    exit(1);
}

struct termios orig_termios;

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");
}

/**
 * Enable raw mode for the terminal
 * The ECHO feature causes each key you type to be printed to the terminal, so you can see what you’re typing.
 * Terminal attributes can be read into a termios struct by tcgetattr().
 * After modifying them, you can then apply them to the terminal using tcsetattr().
 * The TCSAFLUSH argument specifies when to apply the change: in this case, it waits for all pending output to be written to the terminal, and also discards any input that hasn’t been read.
 * The c_lflag field is for “local flags”
*/
void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); // Turn off Ctrl-C, Ctrl-Z and Ctrl-V signals.
    raw.c_iflag &= ~(ICRNL | IXON); // Turn off Ctrl-S which pauses output from being sent. (Ctrl-Q resumes output), and fix Ctrl-M from being read as 13 (carriage return, not \n -> 10).
//    raw.c_oflag &= ~(OPOST); // Prevent all post-processing of output (like turning "\n" into "\r\n"). This is just convention.
    raw.c_cc[VMIN] = 1; // Min number of bytes of input needed before read() can return.
//    raw.c_cc[VTIME] = 0; // TIMEOUT FOR READ().
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

void neonate(char* token) {
    char* endPtr;
    long time_arg = 0;

    if(!token) {
        fprintf(stderr, BRED "ERROR: Missing arguments!\nCommand usage: neonate -n [time_arg]\n" CRESET);
        return;
    }
    else if(token[0] != '-' || token[1] != 'n'){
        fprintf(stderr, BRED "ERROR: Invalid arguments!\nCommand usage: neonate -n [time_arg]\n" CRESET);
        return;
    }
    else{
        token = strtok(NULL, delimiters);
        if(!token) {
            fprintf(stderr, BRED "ERROR: Invalid arguments!\nCommand usage: neonate -n [time_arg]\n" CRESET);
            return;
        }
        time_arg = strtol(token, &endPtr, 10);
        if(endPtr == token || time_arg > INT_MAX || time_arg < 0 || endPtr[0] == '.') {
            fprintf(stderr, BRED "ERROR: Invalid time_arg! Please input a valid 32-bit non-negative integer for time_arg.\nCommand usage: neonate -n [time_arg]\n" CRESET);
            return;
        }
    }

    int childPID = fork();
    if(childPID > 0) {
        setbuf(stdout, NULL);
        enableRawMode();

        char c = '\0';
        while(1) {
            if (read(STDIN_FILENO, &c, 1) == 1 && c == 'x') {
                kill(childPID, SIGKILL);
                break;
            }
        }
        disableRawMode();
    }
    else if(childPID == 0){
        // Process the actual printing.
        printf(BGRN "Press x to stop!\n" CRESET);
        while(1) {
            int reqPID = latestPID();
            if (reqPID == -1) return;

            printf(BBLU "%d\n" CRESET, reqPID);
            fflush(stdout);
            sleep(time_arg);
        }
        exit(EXIT_FAILURE); // Just in case.
    }
    else fprintf(stderr, BRED "ERROR: Failed to fork(). Kindly ensure that the maximum number of child processes haven't been reached.\n" CRESET);
}
