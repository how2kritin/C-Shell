#include "../main/headers.h"

// Reference: Regarding dirent.h -> https://pubs.opengroup.org/onlinepubs/7908799/xsh/dirent.h.html
// About scandir() (preferred over readdir(), as readdir() doesn't ensure any particular order.) -> man scandir
// Filetype testing -> https://man7.org/linux/man-pages/man2/lstat.2.html in examples.
// Permissions testing -> https://linux.die.net/man/1/chmod
// ALL ABOUT STAT -> https://www.ibm.com/docs/en/i/7.2?topic=ssw_ibm_i_72/apis/stat.html
// LSTAT -> Basically the same as stat, but works on symlink itself instead of the file it is linked to -> https://www.ibm.com/docs/en/i/7.4?topic=ssw_ibm_i_74/apis/lstat.html
// time.h for localtime_r and struct tm -> https://pubs.opengroup.org/onlinepubs/7908799/xsh/time.h.html
// For colors in terminal -> https://www.theurbanpenguin.com/4184-2/
// For block count -> https://man7.org/linux/man-pages/man2/lstat.2.html AND https://askubuntu.com/questions/1252657/why-is-total-stat-block-size-of-a-directory-twice-of-ls-block-size-of-a-director

// 4 cases arise -> If upon arrival,
// 1. Token is empty -> no flag and no directory => don't show hidden files; peek at CWD (not necessarily home of the shell.)
// 2. You got only flags -> no directory => do appropriate flag function in CWD.
// 3. You got only directory -> no flags => don't show hidden files; peek at given directory.
// 4. You have both flags and directory => do appropriate flag function in given directory.

void peekHelper(char* token){
    bool flag_a = false, flag_l = false;

    if(!token) { // No directory name given, and no flags given.
        peek(NULL, flag_a, flag_l);
        return;
    }

    // To process flags.
    while(token && token[0] == '-'){
        if(token[1] == '\0') {fprintf(stderr, BRED "ERROR: Flag operator found, but missing flags!\nUsage: peek <flags> <path/name>, where flags are -l, -a.\n" CRESET); return;}
        for(int i = 1; token[i] != '\0'; i++){
            if(token[i] == 'l') flag_l = true;
            else if(token[i] == 'a') flag_a = true;
            else {fprintf(stderr, BRED "ERROR: Invalid flags!\nUsage: peek <flags> <path/name>, where flags are -l, -a.\n" CRESET); return;}
        }
        token = strtok(NULL, delimiters);
    }

    if(!token) { // No directory name given.
        peek(NULL, flag_a, flag_l);
        return;
    }

    if(token[0] == '~') {
        char finalPath[8192] = {};
        strcat(finalPath, homeDirAbsPath);
        strcat(finalPath, token+1); // Ignore '~' and copy rest (if anything exists).
        peek(finalPath, flag_a, flag_l);
        return;
    }

    else peek(token, flag_a, flag_l); // Default case.
}

void peek(char* directory, bool flag_a, bool flag_l){
    int returnVal = 0;
    char reqDir[2*PATH_MAX];
    if(!directory) getcwd(reqDir, PATH_MAX);
    else strcpy(reqDir, directory);

    struct dirent** info; // To store name of the file in directory.
    struct stat stats; // To retrieve information about a file.
    struct passwd* owner; // To store the details of the owner of file.
    struct group* grp; // To store the details of group of file.
    struct tm timeModif; // To store the last modified date and time of file.

    // Additional conversions for time.h -> struct tm values to string:
    static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    // Don't need to opendir. scandir is enough.
//    DIR* dir = opendir(reqDir);
//    if(!dir) {
//        perror("Unable to open this directory");
//        return;
//    }

    returnVal = scandir(reqDir, &info, NULL, alphasort); // Returns -1 if failed, number of files if successful.
    if(returnVal == -1) {
        perror("Unable to scan this directory");
        return;
    }

    // To calculate total block size of the directory:
    if(flag_l) {
        long blockSize = 0;

        for (int i = 0; i < returnVal; i++) {
            if (!flag_a && info[i]->d_name[0] == '.')
                continue; // If not checking for hidden files, then skip over them.

            char filePath[PATH_MAX] = {};
            strcat(filePath, reqDir);
            strcat(filePath, "/");
            strcat(filePath, info[i]->d_name);
            lstat(filePath, &stats); // Store info about file here.

            blockSize += (stats.st_blocks); // Get it in terms of 512 byte chunks.
        }

        printf("total %ld\n", blockSize);
    }

    for(int i = 0; i < returnVal; i++) {
        if (!flag_a && info[i]->d_name[0] == '.') continue; // If not checking for hidden files, then skip over them.

        // Some pre-requisites.
        char filePath[PATH_MAX] = {}, tempBuffer[PATH_MAX] = {};
        strcat(filePath, reqDir);
        strcat(filePath, "/");
        strcat(filePath, info[i]->d_name);
        lstat(filePath, &stats); // Store info about file here.
        unsigned int checkType = stats.st_mode & S_IFMT;

        if(!flag_l) {
            if(checkType == S_IFDIR) printf(BBLU); // Blue for directories.
            else if(checkType == S_IFLNK) printf(BCYN); // Cyan for symlinks.
            else if(stats.st_mode & S_IXUSR) printf(BGRN); // Green for executable files -> Must have execute permission in OWNER.
            else printf(BWHT); // White for files.
            printf(" %s" CRESET, info[i]->d_name); // Print and reset to default colour.
        }
        else{ // Process -l flag details here, just like in Bash.
            // For the file type:
            if(checkType == S_IFBLK) printf("b"); // Block Device
            else if(checkType == S_IFCHR) printf("c"); // Character Device
            else if(checkType == S_IFDIR) printf("d"); // Directory
            else if(checkType == S_IFIFO) printf("p"); // FIFO/pipe
            else if(checkType == S_IFLNK)  printf("l"); // Symlink
            else if(checkType == S_IFSOCK) printf("s"); // Socket
            else printf("-"); // Special file type hasn't been determined, so, must be a regular file.

            // For the permissions:
            printf((stats.st_mode & S_IRUSR) ? "r" : "-");
            printf((stats.st_mode & S_IWUSR) ? "w" : "-");
            printf((stats.st_mode & S_IXUSR) ? "x" : "-");
            printf((stats.st_mode & S_IRGRP) ? "r" : "-");
            printf((stats.st_mode & S_IWGRP) ? "w" : "-");
            printf((stats.st_mode & S_IXGRP) ? "x" : "-");
            printf((stats.st_mode & S_IROTH) ? "r" : "-");
            printf((stats.st_mode & S_IWOTH) ? "w" : "-");
            printf((stats.st_mode & S_IXOTH) ? "x" : "-");

            // Number of hard links: It is the number of directories having a reference to this file.
            printf(" %4lu", stats.st_nlink); // Pad by 4 units.

            // Owner:
            owner = getpwuid(stats.st_uid);
            printf(" %-15s", owner->pw_name); // Pad by 15 units and left align.

            // Group:
            grp = getgrgid(stats.st_gid);
            printf(" %-15s", grp->gr_name); // Pad by 15 units and left align.

            // Size in bytes:
            printf(" %9zu", stats.st_size); // Pad by 5 units.

            // Last modified (exactly how Bash does it -> %h %d %H:%M)
            localtime_r(&stats.st_mtime, &timeModif);
            printf(" %3s %2d ", months[timeModif.tm_mon], timeModif.tm_mday); // Pad by 3 char for Month name, and pad by 2 char for date.

            // If the in which this file was made is NOT the same as the current year, then it is illogical to display it's date and time instead of year of modification.
            // So, I display the year of modification instead, in that case.
            time_t now = time(NULL);
            struct tm* currTime = localtime(&now);
            if(timeModif.tm_year != currTime->tm_year) printf("%-5d", timeModif.tm_year + 1900); // DO PADDING OF 5 CHARS AND LEFT ALIGN!
            else printf("%02d:%02d", timeModif.tm_hour, timeModif.tm_min); // 0 pad for 2 digits of integers, for hour and min.

            // Filename -> Print appropriate colour depending on file type.
            if(checkType == S_IFDIR) printf(BBLU); // Blue for directories.
            else if(checkType == S_IFLNK) printf(BCYN); // Cyan for symlinks.
            else if(stats.st_mode & S_IXUSR) printf(BGRN); // Green for executable files -> Must have execute permission in OWNER.
            else printf(BWHT); // White for files.
            printf(" %s" CRESET, info[i]->d_name); // Print and reset to default colour.
            if(checkType == S_IFLNK) { // For symlinks, display the path that they are linked to.
                readlink(filePath, tempBuffer, PATH_MAX);
                printf(BCYN " -> %s" CRESET, tempBuffer);
            }

            printf("\n");
        }
    }
    if(!flag_l) printf("\n"); // Print newline only at end if no -l flag, instead of printing it after every file name.

//    closedir(dir);
}