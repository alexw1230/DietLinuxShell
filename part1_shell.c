/*
 * ESE 333 Project 2 - Part 1: A Basic Shell
 *
 * Reads one line at a time, splits it into words, and runs it as a single
 * command: fork() a child, the child replaces itself with the program via
 * execvp(), and the parent waits for it with waitpid() before prompting again.
 * No pipes, redirection, or background jobs yet (those come in Part 3).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>   /* waitpid() */
#include <unistd.h>     /* fork(), execvp() */

#define MAX_LINE 1024   /* size of the input buffer           */
#define MAX_ARGS 64     /* size of argv, including final NULL */

void parse(char *line, char **argv)
{
    while (*line != '\0') {       /* if not the end of line ....... */ 
        while (*line == ' ' || *line == '\t' || *line == '\n')
            *line++ = '\0';       /* replace white spaces with 0    */
        if (*line != '\0')
            *argv++ = line;       /* save the argument position     */
        while (*line != '\0' && *line != ' ' && 
               *line != '\t' && *line != '\n') 
            line++;               /* skip the argument until ...    */
    }
    *argv = NULL;                 /* mark the end of argument list  */
}

void execute(char **argv)
{
    pid_t pid;
    int status;
    fflush(stdout);

    pid = fork();
    if (pid < 0) { //failed fork
        perror("fork");
        return;
    }

    if (pid == 0) {
        //child
        execvp(argv[0], argv);

        //failed exec
        if (errno == ENOENT)
            fprintf(stderr, "%s: command not found\n", argv[0]);
        else
            perror(argv[0]);
        exit(127);
    }

    //wait for child exit
    if (waitpid(pid, &status, 0) < 0)
        perror("waitpid");
}

int main(void)
{
    char line[MAX_LINE];                    /* the input line            */
    char *argv[MAX_ARGS];                   /* the command line argument */

    fputs("Type 'exit' to quit\n\n", stdout);

    while (1) {                             /* repeat until done ....    */
        fputs("Shell -> ", stdout);         /*   display a prompt        */
        fflush(stdout);                     /*   make sure it shows now  */

        /* fgets returns NULL at end of input (Ctrl-D or end of a piped script). */
        if (fgets(line, sizeof(line), stdin) == NULL) {
            fputs("\n", stdout);
            break;
        }

        parse(line, argv);                  /*   parse the line          */

        if (argv[0] == NULL)                /*   blank line: re-prompt   */
            continue;
        if (strcmp(argv[0], "exit") == 0)   /*   is it an "exit"?        */
            break;

        execute(argv);                      /* otherwise, execute it     */
    }
    return 0;
}