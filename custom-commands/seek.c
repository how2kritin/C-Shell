#include "../main/headers.h"

// Global variables to store most recent state.
char mostRecentFoundPath[4096] = {};
bool mostRecentIsDir = false, mostRecentHasRead = false, mostRecentHasExec = false;

void seekHandler(char* token){
    bool onlyFiles = false, onlyDir = false, singleFile = false;

    // To process flags.
    while(token && token[0] == '-'){
        if(token[1] == '\0') {fprintf(stderr, BRED "ERROR: Flag operator found, but missing flags!\nUsage: seek <flags> <search> <target_directory>, where flags are -d, -f or -e.\n" CRESET); return;}
        for(int i = 1; token[i] != '\0'; i++){
            if(token[i] == 'd') onlyDir = true;
            else if(token[i] == 'f') onlyFiles = true;
            else if(token[i] == 'e') singleFile = true;
            else {fprintf(stderr, BRED "ERROR: Invalid flags!\nUsage: seek <flags> <search> <target_directory>, where flags are -d, -f or -e.\n" CRESET); return;}
        }
        token = strtok(NULL, delimiters);
    }
    if(onlyDir && onlyFiles) {fprintf(stderr, BRED "ERROR: Invalid flags!\nUsage: seek <flags> <search> <target_directory>, where flags are -d, -f or -e.\n" CRESET); return;}

    char* nameOfFile = token;
    if(nameOfFile == NULL) {
        fprintf(stderr, BRED "ERROR: Missing arguments!\nUsage: seek <flags> <search> <target_directory>, where flags are -d, -f or -e.\n" CRESET);
        return;
    }

    // Process path here: Basically, change directory into the required path, and get all the relative file paths from there, and go back to the previous CWD before the next prompt.
    token = strtok(NULL, delimiters);
    char cwdPath[PATH_MAX] = {}, processPath[PATH_MAX] = {};
    getcwd(cwdPath, PATH_MAX);
    if(token) {
        if (token[0] == '~') {
            strcat(processPath, homeDirAbsPath);
            strcat(processPath, token + 1); // Ignore '~' and copy rest (if anything exists).
        } else strcat(processPath, token);
        int returnVal = chdir(processPath);
        if(returnVal == -1){
            perror("Failed to open directory to search");
            return;
        }
    }
    int countNumFound = seek(onlyFiles, onlyDir, singleFile, nameOfFile, ".");
    bool finalDirChanged = false;
    if(countNumFound == 0) printf(BYEL "No match found!\n" CRESET);

    // Process -e flag:
    else if(singleFile && countNumFound == 1){
        if(!mostRecentIsDir && !mostRecentHasRead) fprintf(stderr, BRED "ERROR: Missing permissions for task!\n" CRESET);
        else if(mostRecentIsDir && !mostRecentHasExec) fprintf(stderr, BRED "ERROR: Missing permissions for task!\n" CRESET);
        else{
            if(mostRecentIsDir) chdir(mostRecentFoundPath), finalDirChanged = true;
            else {
                FILE* fPtr = fopen(mostRecentFoundPath, "r");
                char ch = fgetc(fPtr);
                while (ch != EOF) printf ("%c", ch), ch = fgetc(fPtr);
                fclose(fPtr);
            }
        }
    }

    if(!finalDirChanged) chdir(cwdPath);
}

int seek(bool onlyFiles, bool onlyDir, bool singleFile, char* nameOfFile, char* path){
    int returnVal, countNumFilesFound = 0;
    struct dirent** info; // To store name of the file in directory.
    struct stat stats; // To retrieve information about a file.

    // Don't need to open dir. scandir is enough.
//    DIR* dir = opendir(path);
//    if(!dir) {
//        perror("Unable to open this directory");
//        printf(BYEL "%s directory does not have execute permissions! Files having \"%s\" as a prefix in this directory will be displayed, but those in sub-directories of this directory won't.\n" CRESET, )
//        return false;
//    }

    returnVal = scandir(path, &info, NULL, alphasort); // Returns -1 if failed, number of files if successful.
    if(returnVal == -1) {
        perror("scandir failed");
        return false;
    }

    for(int i = 0; i < returnVal; i++) {
        if (strcmp(info[i]->d_name, ".") == 0 || strcmp(info[i]->d_name, "..") == 0) continue; // Be sure not to recursively go back to parent directory, or infinitely recurse in the same directory.

        // Some pre-requisites.
        char filePath[PATH_MAX] = {};
        strcat(filePath, path);
        strcat(filePath, "/");
        strcat(filePath, info[i]->d_name);
        lstat(filePath, &stats); // Store info about file here.
        unsigned int checkType = stats.st_mode & S_IFMT;

        // For the file type:
        if(checkType == S_IFDIR) {
            if(!(stats.st_mode & S_IRUSR)) {
                printf(BYEL "%s directory does not have read permissions! Skipping search of files of this directory.\n" CRESET, filePath);
                continue;
            }
            else if (!(stats.st_mode & S_IXUSR)) printf(BYEL "%s directory does not have execute permissions! Files having \"%s\" as a prefix in this directory will be displayed, but those in sub-directories of this directory won't.\n" CRESET, filePath, nameOfFile);
            countNumFilesFound = countNumFilesFound + seek(onlyFiles, onlyDir, singleFile, nameOfFile, filePath); // Directory, so, recursively call the function.
        }

        unsigned long prefixLen = strlen(nameOfFile);
        if(strncmp(info[i]->d_name, nameOfFile, prefixLen) == 0) { // if info[i]->d_name has nameOfFile as prefix:
            if(checkType == S_IFDIR) {
                if(onlyFiles) continue;
                printf(BBLU);  // Blue for directories.
                mostRecentIsDir = true;
            }
            else {
                if(onlyDir) continue;
                printf(BGRN); // Green for files.
                mostRecentIsDir = false;
            }
            countNumFilesFound++;
            strcpy(mostRecentFoundPath, filePath);
            mostRecentHasRead = (stats.st_mode & S_IRUSR);
            mostRecentHasExec = (stats.st_mode & S_IXUSR);
            printf("%s\n" CRESET, filePath); // Print and reset to default colour.
        }
    }
//    closedir(dir);
    return countNumFilesFound;
}