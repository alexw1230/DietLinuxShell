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

/*
 * Split line into words in place. Every space, tab, or newline is overwritten
 * with '\0', and argv[i] points at the start of each word, so no copying or
 * malloc is needed. The list ends with a NULL pointer, which is what execvp
 * expects.
 */
void parse(char *line, char **argv)
{
    int argc = 0;

    while (*line != '\0') {                 /* if not the end of line ....... */
        while (*line == ' ' || *line == '\t' || *line == '\n')
            *line++ = '\0';                 /* replace white spaces with 0    */
        if (*line != '\0' && argc < MAX_ARGS - 1)
            argv[argc++] = line;            /* save the argument position     */
        while (*line != '\0' && *line != ' ' &&
               *line != '\t' && *line != '\n')
            line++;                         /* skip the argument until ...    */
    }
    argv[argc] = NULL;                      /* mark the end of argument list  */
}

/*
 * Run one command and wait for it to finish.
 */
void execute(char **argv)
{
    pid_t pid;
    int status;

    /*
     * Flush stdout before forking. If output is going to a pipe or file (as
     * when a grading script runs the shell), stdio holds text like the
     * "Shell -> " prompt in a buffer instead of writing it right away.
     * Unflushed, the shell's text comes out after the commands' output, and
     * because fork() copies the buffer, a child whose execvp fails would
     * print it a second time when it calls exit().
     */
    fflush(stdout);

    pid = fork();
    if (pid < 0) {                          /* fork failed: no child exists */
        perror("fork");
        return;
    }

    if (pid == 0) {
        /*
         * Child. execvp searches PATH for argv[0] and replaces this process
         * with that program, passing argv as its arguments. If it succeeds
         * it never returns.
         */
        execvp(argv[0], argv);

        /*
         * Reaching this line means execvp failed. The child must exit here;
         * otherwise it would return into main() and a second copy of the
         * shell would keep reading commands.
         */
        if (errno == ENOENT)
            fprintf(stderr, "%s: command not found\n", argv[0]);
        else
            perror(argv[0]);
        exit(127);                          /* same status bash uses */
    }

    /* Parent: block until this child exits, so output never overlaps the next prompt. */
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