#include <fcntl.h>   /* For file-mode definitions (open flags)      */
#include <stdio.h>   /* For fprintf, sprintf, perror                */
#include <stdlib.h>  /* For exit                                    */
#include <unistd.h>  /* For read/write/lseek/close/unlink/getpid    */

/* Enumerator */
enum { FALSE, TRUE };           /* Standard false and true values    */
enum { STDIN, STDOUT, STDERR }; /* Standard I/O-channel indices      */

/* #define Statements */
#define BUFFER_SIZE 4096    /* Copy buffer size                      */
#define NAME_SIZE   32      /* Temp file-name buffer size            */
#define MAX_LINES   100000  /* Max lines in file                     */

/* Globals */
char *fileName = NULL;        /* Points to file name                 */
char  tmpName[NAME_SIZE];     /* Name of temp file for stdin copy    */
int   charOption   = FALSE;   /* TRUE if the '-c' option is used     */
int   standardInput = FALSE;  /* TRUE if reading from stdin          */
int   lineCount    = 0;       /* Total number of lines in input      */
int   lineStart[MAX_LINES];   /* Byte offset where each line starts  */
int   fileOffset   = 0;       /* Current position in input           */
int   fd;                     /* File descriptor of input            */

/* Function prototypes (ANSI style, so the compiler sees signatures) */
void parseCommandLine(int argc, char *argv[]);
void processOptions(char *str);
void usageError(void);
void pass1(void);
void trackLines(char *buffer, int charsRead);
void pass2(void);
void processLine(int i);
void reverseLine(char *buffer, int size);
void fatalError(void);

/**************************************************************/
int main(int argc, char *argv[])
{
    parseCommandLine(argc, argv); /* Parse command line          */
    pass1();                      /* First pass: index the lines */
    pass2();                      /* Second pass: print reversed */
    return 0;                     /* Done                        */
}
/**************************************************************/
void parseCommandLine(int argc, char *argv[])
/* Parse command-line arguments */
{
    int i;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-')
            processOptions(argv[i]);
        else if (fileName == NULL)
            fileName = argv[i];
        else
            usageError();           /* Too many file names */
    }
    standardInput = (fileName == NULL);
}
/**************************************************************/
void processOptions(char *str)
/* Parse the letters of one '-xyz' option string */
{
    int j;
    for (j = 1; str[j] != '\0'; j++) {
        switch (str[j]) {           /* Switch on command-line flag */
            case 'c':
                charOption = TRUE;
                break;
            default:
                usageError();
                break;
        }
    }
}
/**********************************************************/
void usageError(void)
{
    fprintf(stderr, "Usage: reverse -c [filename]\n");
    exit(1);
}
/*********************************************************/
void pass1(void)
/* Perform first scan through file, recording line offsets */
{
    int tmpfd = -1, charsRead, charsWritten;
    char buffer[BUFFER_SIZE];

    if (standardInput) {            /* Read from standard input */
        fd = STDIN;
        sprintf(tmpName, ".rev.%d", getpid());  /* Unique temp name */
        /* Create temporary file to store a copy of stdin */
        tmpfd = open(tmpName, O_CREAT | O_RDWR, 0600);
        if (tmpfd == -1) fatalError();
    } else {                        /* Open the named file */
        fd = open(fileName, O_RDONLY);
        if (fd == -1) fatalError();
    }

    lineStart[0] = 0;               /* Offset of first line */

    while (TRUE) {                  /* Read all input */
        charsRead = read(fd, buffer, BUFFER_SIZE);
        if (charsRead == 0) break;          /* EOF   */
        if (charsRead == -1) fatalError();  /* Error */
        trackLines(buffer, charsRead);      /* Record line starts */
        if (standardInput) {                /* Mirror stdin to temp */
            charsWritten = write(tmpfd, buffer, charsRead);
            if (charsWritten != charsRead) fatalError();
        }
    }

    /* Account for a trailing line that has no terminating '\n'.     */
    /* Without this, the last unterminated line would be lost.       */
    if (fileOffset > lineStart[lineCount])
        ++lineCount;
    lineStart[lineCount] = fileOffset;      /* End boundary of input */

    if (standardInput) fd = tmpfd;          /* pass2 reads the copy  */
}
/*************************************************************/
void trackLines(char *buffer, int charsRead)
/* Store the byte offset at which each line starts */
{
    int i;
    for (i = 0; i < charsRead; i++) {
        ++fileOffset;               /* Update current file position */
        if (buffer[i] == '\n')
            lineStart[++lineCount] = fileOffset;
    }
}
/************************************************/
void pass2(void)
/* Scan input file again, displaying lines in reverse order */
{
    int i;
    for (i = lineCount - 1; i >= 0; i--)
        processLine(i);

    close(fd);                      /* Close input file */
    if (standardInput) unlink(tmpName);  /* Remove temp file */
}
/*************************************************/
void processLine(int i)
/* Read line i and display it (optionally char-reversed) */
{
    int  charsRead;
    char buffer[BUFFER_SIZE];

    lseek(fd, lineStart[i], SEEK_SET);          /* Seek to line start */
    charsRead = read(fd, buffer, lineStart[i + 1] - lineStart[i]);
    if (charOption) reverseLine(buffer, charsRead);  /* '-c' option   */
    write(STDOUT, buffer, charsRead);           /* Write to stdout   */
}
/*********************************************************/
void reverseLine(char *buffer, int size)
/* Reverse the characters in buffer, keeping a trailing '\n' last */
{
    int start = 0, end = size - 1;
    char tmp;

    if (size > 0 && buffer[end] == '\n') --end; /* Leave newline */
    while (start < end) {                        /* Swap pairwise */
        tmp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = tmp;
        ++start;
        --end;
    }
}
/*********************************************************/
void fatalError(void)
{
    perror("reverse:");             /* Describe the system error */
    exit(1);
}
