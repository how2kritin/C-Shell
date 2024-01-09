#include "../main/headers.h"

// References: To get the status of a process -> https://linux.die.net/man/2/waitpid AND man waitpid
// To get process group -> https://www.ibm.com/docs/en/zos/2.1.0?topic=functions-getpgid-get-process-group-id
// To get execPath -> https://man7.org/linux/man-pages/man2/readlink.2.html AND man readlink
// To get process status and virtual memory -> Read it from /proc/<pid>/stat AND man proc -> https://man7.org/linux/man-pages/man5/proc.5.html AND https://stackoverflow.com/questions/55195636/how-to-read-all-background-processes-from-proc-directory
// (5) pgrp  %d -> The process group ID of the process AND (8) tpgid  %d -> The ID of the foreground process group of the controlling terminal of the process.
// If pgrp == tpgid, then the process is in the foreground. Else, it is in the background. Basically, if process group ID is same as terminal group ID, then it is foreground. Else, it is background.
// Color coding in C -> https://www.theurbanpenguin.com/4184-2/

void proclore(char* token) {
    int pid, pgrp, tpgid;
    unsigned long vsize;
    char status[3] = {}, execPath[PATH_MAX] = {}, tempBuffer[PATH_MAX] = {};

    if (!token) pid = getpid(); // Handle the case where no pid is given. By default, take it as this C shell itself.
    else pid = atoi(token);

    sprintf(tempBuffer, "/proc/%d/stat", pid);
    FILE* filePtr = fopen(tempBuffer, "r");
    if(!filePtr) {
        fprintf(stderr, BRED "ERROR: A process with this process ID doesn't exist. Please ensure that the entered pid is valid!\n" CRESET);
        return;
    }
    fscanf(filePtr, "%*d %*s %s %*s %d %*s %*s %d %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %lu", status, &pgrp, &tpgid, &vsize); // * before format specifier will not store that value. Basically, ignore that value.
    // We will use pgrp and tpgid to see if process is background or foreground.
    if(pgrp == tpgid) status[1] = '+', status[2] = '\0';
    fclose(filePtr);

    sprintf(tempBuffer, "/proc/%d/exe", pid);
    ssize_t returnVal = readlink(tempBuffer, execPath, PATH_MAX);
    if (returnVal == -1) {
        perror("readlink failed while opening /proc/pid/exe");
        return;
    }
    memset(tempBuffer, 0, PATH_MAX*sizeof(char));
    getrelpath(tempBuffer, execPath); // This function is defined in prompt.c.

    printf("pid : %d\n", pid);
    printf("Process Status : %s\n", status);
    printf("Process Group : %d\n", pgrp);
    printf("Virtual Memory : %lu bytes\n", vsize);
    printf("Executable Path : %s\n", tempBuffer);
}