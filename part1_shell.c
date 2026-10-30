#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

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
        //execute command
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
    char line[1024];         /* the input line                 */
    char *argv[64];          /* the command line argument      */

    while (1)
    {                                     /* repeat until done ....         */
        fputs("Shell -> ", stdout);       /*   display a prompt             */
        if (fgets(line, 1024, stdin) == NULL) /* read in the command line   */
            exit(0);                      /*   end of input: quit           */
        parse(line, argv);                /*   parse the line               */
        if (argv[0] == NULL)              /*   blank line: prompt again     */
            continue;
        if (strcmp(argv[0], "exit") == 0) /* is it an "exit"?     */
            exit(0);                      /*   exit if it is                */
        execute(argv);                    /* otherwise, execute the command */
    }
    return 0;
}