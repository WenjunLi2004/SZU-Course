#include <stdio.h>          /* For printf, fprintf */
#include <string.h>         /* For strcmp */
#include <ctype.h>         /* For isdigit */
#include <fcntl.h>          /* For O_RDONLY */
#include <dirent.h>  /* For getdents */
#include<sys/syscall.h>
#include <sys/stat.h>     /* For IS macros */
#include <time.h>          /* For localtime, asctime */
#include <unistd.h>
#include <stdlib.h>

/* #define Statements */
#define MAX_FILES 100
#define MAX_FILENAME 50
#define NOT_FOUND (-1)
#define FOREVER (-1)
#define DEFAULT_DELAY_TIME 10
#define DEFAULT_LOOP_COUNT FOREVER

/* Booleans */
enum {
    FALSE, TRUE
};

/* Status structure, one per file. */
struct statStruct {
    char fileName[MAX_FILENAME];  /* File name */
    int lastCycle, thisCycle;   /* To detect changes */
    struct stat status;   /* Information from stat() */
};

/* Globals */
char *fileNames[MAX_FILES];  /* One per file on command line */
int fileCount;  /* Count of files on command line */
struct statStruct stats[MAX_FILES];  /* One per matching file */
int loopCount = DEFAULT_LOOP_COUNT;  /* Number of times to loop */
int delayTime = DEFAULT_DELAY_TIME; /* Seconds between loops */
void parseCommandLine(int argc, char *argv[]);

void usageError();

void monitorLoop();

void monitorFiles();

void processDirectory(char *dirName);

void updateStat(char *fileName, struct stat *statBuf);

int findEntry(char *fileName);

void updateEntry(int index, struct stat *statBuf);

void printEntry(int index);

void printStat(struct stat *statBuf);

void fatalError();

int nextFree();

int getNumber(char *str, int *i);

int addEntry(char *fileName, struct stat *statBuf);

void monitorFile(char *fileName);

/****************************************************/
void processOptions(char *str) {  /* Parse options */
    int j;
    for (j = 1; str[j]; j++) {
        switch (str[j]) {/* Switch on option letter */
            case 't':
                delayTime = getNumber(str, &j);
                break;
            case 'l':
                loopCount = getNumber(str, &j);
                break;
        }
    }
}

int main(int argc, char *argv[]) {
    parseCommandLine(argc, argv); /* Parse command line */
    monitorLoop();  /* Execute main monitor loop */
    return ( /* EXIT_SUCCESS */  0);
}

/****************************************************/
void parseCommandLine(int argc, char *argv[]) {/* Parse command-line arguments */
    int i;
    for (i = 1; ((i < argc) && (i < MAX_FILES)); i++) {
        if (argv[i][0] == '-') processOptions(argv[i]);
        else fileNames[fileCount++] = argv[i];
    }
    if (fileCount == 0) usageError();
}

/****************************************************/
int getNumber(char *str, int *i) {/* Convert a numeric ASCII option to a number */
    int number = 0;
    int digits = 0;   /* Count the digits in the number */
    while (isdigit(str[(*i) + 1])) {/* Convert chars to ints */
        number = number * 10 + str[++(*i)] - '0';
        ++digits;
    }
    if (digits == 0) usageError();  /* There must be a number */
    return (number);
}

/****************************************************/
void usageError() {
    fprintf(stderr, "Usage:monitor - t < seconds > -l < loops > {filename} +\n");
    exit( /* EXIT_FAILURE */  1);
}

/****************************************************/
void monitorLoop() {/* The main monitor loop */
    do {
        monitorFiles();  /* Scan all files */
        fflush(stdout);  /* Flush standard output */
        fflush(stderr);  /* Flush standard error */
        sleep(delayTime);  /* Wait until next loop */
    } while (loopCount == FOREVER || --loopCount > 0);
}

/****************************************************/
void monitorFiles() {/* Process all files */
    int i;
    for (i = 0; i < fileCount; i++)
        monitorFile(fileNames[i]);
    for (i = 0; i < MAX_FILES; i++) {/* Update stat array */
        if (stats[i].lastCycle && !stats[i].thisCycle)
            printf("DELETED %s\n", stats[i].fileName);
        stats[i].lastCycle = stats[i].thisCycle;
        stats[i].thisCycle = FALSE;
    }
}

/****************************************************/
void monitorFile(char *fileName) {/* Process a single file/directory */
    struct stat statBuf;
    mode_t mode;
    int result;
    result = stat(fileName, &statBuf); /* Obtain file status */
    if (result == -1) {/* Status was not available */
        fprintf(stderr, "File %s does not exist. Cannot stat\n", fileName);
        return;
    }
    mode = statBuf.st_mode;  /* Mode of file */
    if (S_ISDIR(mode)) /* Directory */
        processDirectory(fileName);
    else if (S_ISREG(mode) || S_ISCHR(mode) || S_ISBLK(mode))
        updateStat(fileName, &statBuf);  /* Regular file */
}

/******************************************************/
void processDirectory(char *dirName) {/* Process all files in the named directory */
    int fd, charsRead;
    struct dirent dirEntry;
    char fileName[MAX_FILENAME];
    fd = open(dirName, O_RDONLY); /* Open for reading */
    if (fd == -1) fatalError();
    while (TRUE) {/* Read all directory entries */
        charsRead = syscall(SYS_getdents64, fd, &dirEntry, sizeof(struct dirent));
        if (charsRead == -1) fatalError();
        if (charsRead == 0) break;  /* EOF */
        if (strcmp(dirEntry.d_name, ".") != 0 && strcmp(dirEntry.d_name, "..") != 0) { /* Skip .  and  .. */
            sprintf(fileName, "%s/%s", dirName, dirEntry.d_name);
            monitorFile(fileName);  /* Call recursively */
        }
        lseek(fd, dirEntry.d_off, SEEK_SET);  /* Find next entry */
    }
    close(fd);  /* Close directory */
}

/*******************************************************/
void updateStat(char *fileName, struct stat *statBuf) {/* Add a status entry if necessary */
    int entryIndex;
    entryIndex = findEntry(fileName);  /* Find existing entry */
    if (entryIndex == NOT_FOUND)
        entryIndex = addEntry(fileName, statBuf);  /* Add new entry */
    else
        updateEntry(entryIndex, statBuf); /* Update existing entry */
    if (entryIndex != NOT_FOUND)
        stats[entryIndex].thisCycle = TRUE; /* Update status array */
}

/******************************************************/
int findEntry(char *fileName) {/* Locate the index of a named file in the status array */
    int i;
    for (i = 0; i < MAX_FILES; i++)
        if (stats[i].lastCycle && strcmp(stats[i].fileName, fileName) == 0)
            return (i);
    return (NOT_FOUND);
}

int addEntry(char *fileName, struct stat *statBuf) {/* Add a new Entry into the status array */
    int index;
    index = nextFree();  /* Find the next free entry */
    if (index == NOT_FOUND) return (NOT_FOUND);  /* None left */
    strcpy(stats[index].fileName, fileName);  /* Add filename */
    stats[index].status = *statBuf;  /* Add status information */
    printf("ADDED ");  /* Notify standard output */
    printEntry(index);  /* Display status information */
    return (index);
}

/****************************************************/
int nextFree() {/* Return the next free index in the status array */
    int i;
    for (i = 0; i < MAX_FILES; i++)
        if (!stats[i].lastCycle && !stats[i].thisCycle) return (i);
    return (NOT_FOUND);
}

/*****************************************************/
void updateEntry(int index, struct stat *statBuf) {/* Display information if the file has been modified */
    if (stats[index].status.st_mtime != statBuf->st_mtime) {
        stats[index].status = *statBuf;  /* Store stat information */
        printf("CHANGED ");   /* Notify standard output */
        printEntry(index);
    }
}

/*************************************************/
void printEntry(int index) {/* Display an entry of the status array */
    printf("%s   ", stats[index].fileName);
    printStat(&stats[index].status);
}

/*************************************************/
void printStat(struct stat *statBuf) {/* Display a status buffer */
    printf("size %lu bytes, mod.time = %s", statBuf->st_size,
           asctime(localtime(&statBuf->st_mtime)));
}

/*************************************************/
void fatalError() {
    perror("monitor: ");
    exit( /* EXIT_FAILURE */  1);
} 
