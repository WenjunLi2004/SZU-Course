#include <stdio.h>        /* For printf, fprintf, sprintf            */
#include <stdlib.h>       /* For exit                                */
#include <string.h>       /* For strcmp, strcpy                      */
#include <ctype.h>        /* For isdigit                             */
#include <fcntl.h>        /* For O_RDONLY                            */
#include <unistd.h>       /* For sleep                               */
#include <dirent.h>       /* For opendir/readdir/closedir            */
#include <sys/stat.h>     /* For stat() and the S_IS* macros         */
#include <sys/types.h>    /* For mode_t                              */
#include <time.h>         /* For localtime, asctime                  */

/* #define Statements */
#define MAX_FILES          100
#define MAX_FILENAME       256
#define NOT_FOUND          -1
#define FOREVER            -1
#define DEFAULT_DELAY_TIME 10
#define DEFAULT_LOOP_COUNT FOREVER

/* Booleans */
enum { FALSE, TRUE };

/* Status structure, one per matching file. */
struct statStruct {
    char fileName[MAX_FILENAME]; /* File name                       */
    int  lastCycle, thisCycle;   /* Presence flags, to detect change */
    struct stat status;          /* Information from stat()          */
};

/* Globals */
char *fileNames[MAX_FILES];           /* One per file on command line */
int   fileCount;                      /* Count of command-line files  */
struct statStruct stats[MAX_FILES];   /* One per matching file        */
int   loopCount = DEFAULT_LOOP_COUNT; /* Number of times to loop      */
int   delayTime = DEFAULT_DELAY_TIME; /* Seconds between loops        */

/* Function prototypes */
void processOptions(char *str);
void parseCommandLine(int argc, char *argv[]);
int  getNumber(char *str, int *i);
void usageError(void);
void monitorLoop(void);
void monitorFiles(void);
void monitorFile(char *fileName);
void processDirectory(char *dirName);
void updateStat(char *fileName, struct stat *statBuf);
int  findEntry(char *fileName);
int  addEntry(char *fileName, struct stat *statBuf);
int  nextFree(void);
void updateEntry(int index, struct stat *statBuf);
void printEntry(int index);
void printStat(struct stat *statBuf);
void fatalError(void);

/****************************************************/
void processOptions(char *str)
/* Parse options such as -t<seconds> or -l<loops> */
{
    int j;
    for (j = 1; str[j] != '\0'; j++) {
        switch (str[j]) {           /* Switch on option letter */
            case 't':
                delayTime = getNumber(str, &j);
                break;
            case 'l':
                loopCount = getNumber(str, &j);
                break;
            default:
                usageError();
                break;
        }
    }
}
/****************************************************/
int main(int argc, char *argv[])
{
    parseCommandLine(argc, argv);   /* Parse command line       */
    monitorLoop();                  /* Execute main monitor loop */
    return 0;
}
/****************************************************/
void parseCommandLine(int argc, char *argv[])
/* Parse command-line arguments */
{
    int i;
    for (i = 1; (i < argc) && (i < MAX_FILES); i++) {
        if (argv[i][0] == '-')
            processOptions(argv[i]);
        else
            fileNames[fileCount++] = argv[i];
    }
    if (fileCount == 0) usageError();
}
/****************************************************/
int getNumber(char *str, int *i)
/* Convert a numeric ASCII option (e.g. the 5 in -t5) to an int */
{
    int number = 0;
    int digits = 0;                 /* Count the digits in the number */

    while (isdigit((unsigned char)str[(*i) + 1])) {
        number = number * 10 + str[++(*i)] - '0';
        ++digits;
    }
    if (digits == 0) usageError();  /* There must be a number */
    return number;
}
/****************************************************/
void usageError(void)
{
    fprintf(stderr, "Usage: monitor -t<seconds> -l<loops> {filename}+\n");
    exit(1);
}
/****************************************************/
void monitorLoop(void)
/* The main monitor loop */
{
    do {
        monitorFiles();             /* Scan all files            */
        fflush(stdout);             /* Flush standard output     */
        fflush(stderr);             /* Flush standard error      */
        sleep(delayTime);           /* Wait until the next loop  */
    } while (loopCount == FOREVER || --loopCount > 0);
}
/****************************************************/
void monitorFiles(void)
/* Process every command-line file, then report deletions */
{
    int i;
    for (i = 0; i < fileCount; i++)
        monitorFile(fileNames[i]);  /* Scan one file/directory   */

    for (i = 0; i < MAX_FILES; i++) {          /* Update stat array */
        if (stats[i].lastCycle && !stats[i].thisCycle)
            printf("DELETED %s\n", stats[i].fileName);

        stats[i].lastCycle = stats[i].thisCycle;
        stats[i].thisCycle = FALSE;
    }
}
/****************************************************/
void monitorFile(char *fileName)
/* Process a single file or directory */
{
    struct stat statBuf;
    mode_t mode;

    if (stat(fileName, &statBuf) == -1) {       /* Status unavailable */
        fprintf(stderr, "Cannot stat %s\n", fileName);
        return;
    }
    mode = statBuf.st_mode;                      /* Mode of file */
    if (S_ISDIR(mode))                           /* Directory    */
        processDirectory(fileName);
    else if (S_ISREG(mode) || S_ISCHR(mode) || S_ISBLK(mode))
        updateStat(fileName, &statBuf);          /* Regular file */
}
/******************************************************/
void processDirectory(char *dirName)
/* Process all files in the named directory */
{
    DIR *dir;
    struct dirent *entry;
    char fileName[MAX_FILENAME];

    dir = opendir(dirName);                      /* Open for reading */
    if (dir == NULL) { fatalError(); }

    while ((entry = readdir(dir)) != NULL) {     /* Read all entries */
        if (strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0) {  /* Skip . and ..    */
            sprintf(fileName, "%s/%s", dirName, entry->d_name);
            monitorFile(fileName);               /* Recurse          */
        }
    }
    closedir(dir);                               /* Close directory  */
}
/*******************************************************/
void updateStat(char *fileName, struct stat *statBuf)
/* Add or update the status entry for a file */
{
    int entryIndex;
    entryIndex = findEntry(fileName);            /* Find existing entry */

    if (entryIndex == NOT_FOUND)
        entryIndex = addEntry(fileName, statBuf);   /* Add new entry    */
    else
        updateEntry(entryIndex, statBuf);           /* Update existing  */

    if (entryIndex != NOT_FOUND)
        stats[entryIndex].thisCycle = TRUE;         /* Mark as present  */
}
/******************************************************/
int findEntry(char *fileName)
/* Locate the index of a named file in the status array */
{
    int i;
    for (i = 0; i < MAX_FILES; i++)
        if (stats[i].lastCycle && strcmp(stats[i].fileName, fileName) == 0)
            return i;
    return NOT_FOUND;
}
/******************************************************/
int addEntry(char *fileName, struct stat *statBuf)
/* Add a new entry into the status array */
{
    int index;

    index = nextFree();                          /* Find next free slot */
    if (index == NOT_FOUND) return NOT_FOUND;    /* None left           */
    strcpy(stats[index].fileName, fileName);     /* Add file name       */
    stats[index].status = *statBuf;              /* Add status info     */
    printf("ADDED  ");                           /* Notify stdout       */
    printEntry(index);
    return index;
}
/****************************************************/
int nextFree(void)
/* Return the next free index in the status array */
{
    int i;
    for (i = 0; i < MAX_FILES; i++)
        if (!stats[i].lastCycle && !stats[i].thisCycle) return i;
    return NOT_FOUND;
}
/*****************************************************/
void updateEntry(int index, struct stat *statBuf)
/* Display information if the file has been modified */
{
    if (stats[index].status.st_mtime != statBuf->st_mtime) {
        stats[index].status = *statBuf;          /* Store new stat info */
        printf("CHANGED  ");                     /* Notify stdout       */
        printEntry(index);
    }
}
/*************************************************/
void printEntry(int index)
/* Display one entry of the status array */
{
    printf("%s  ", stats[index].fileName);
    printStat(&stats[index].status);
}
/*************************************************/
void printStat(struct stat *statBuf)
/* Display a status buffer */
{
    printf("size %ld bytes, mod. time = %s",
           (long)statBuf->st_size,
           asctime(localtime(&statBuf->st_mtime)));
}
/*************************************************/
void fatalError(void)
{
    perror("monitor: ");
    exit(1);
}
