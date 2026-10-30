#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

//token types & names
enum {CMD, ARG, FLAG1, FLAG2, SYMBOL};
char *type_names[] = { "CMD", "ARG", "FLAG1", "FLAG2", "SYMBOL" };

//node struct
struct node {
    char *data;
    int type;
    struct node *next;
};

//is char a symbol
int is_symbol(char c) {
    return (c == '|' || c == '<' || c == '>' || c == '&');
}

//create node
struct node *new_node(char *start, int len){
    struct node *n = malloc(sizeof(struct node));
    n->data = malloc(len + 1);
    memcpy(n->data, start, len);
    n->data[len] = '\0';
    n->type = ARG;
    n->next = NULL;
    return n;
}


struct node *parse(char *line)
{
    //linkedlist
    struct node *head = NULL;
    struct node *tail = NULL;
    struct node *n;
    char *start;
    int expect_cmd = 1; //is next wrd a cmd
    int after_redirect = 0; //was redirect?

    while (*line != '\0') { //if not the end of line
        while (*line == ' ' || *line == '\t' || *line == '\n'){
            line++; //skip spaces and newlines
        }
        if (*line == '\0'){
            break;
        }
        start = line;//save token position

        //find the end of the token
        //symbols are one char, words go until a space or symbol
        if (is_symbol(*line)){
            line++;
        } else {
            while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n' && !is_symbol(*line)) {

                //quoted text stays one token
                if (*line == '"' || *line == '\'') {
                    char quote = *line++;
                    while (*line != '\0' && *line != '\n' && *line != quote) {
                        line++;
                    }
                    if (*line == quote) {
                        line++;
                    }
                } else {
                    line++;
                }
            }
        }
        n = new_node(start, line - start);

        //| and & start a new command, < and > come before a file
        if (is_symbol(*start)) {
            n->type = SYMBOL;
            if (*start == '|' || *start == '&') {
                expect_cmd = 1;
            }
            after_redirect = (*start == '<' || *start == '>');
        } else {

            //file, command, flag, or reg argument
            if (after_redirect) {
                n->type = ARG;    //file after redirect
            } else if (expect_cmd) {
                n->type = CMD;
                expect_cmd = 0;
            } else if (n->data[0] == '-' && n->data[1] == '-') {
                n->type = FLAG2;
            } else if (n->data[0] == '-' && n->data[1] != '\0') {
                n->type = FLAG1;
            } else {
                n->type = ARG;
            }
            after_redirect = 0;
        }

        //add node to end of list
        if (head == NULL) {
            head = n;
        } else {
            tail->next = n;
        }
        tail = n;
    }

    //returns null if blank line
    return head;
}

//pt 2 printing, not called here
//print the command list, then each command with what follows it
void print_commands(struct node *list) {
    struct node *n, *a;

    fputs("Commands :", stdout);
    for (n = list; n != NULL; n = n->next) {
        if (n->type == CMD) {
            fputs(" ", stdout);
            fputs(n->data, stdout);
        }
    }
    fputs("\n", stdout);

    for (n = list; n != NULL; n = n->next) {
        if (n->type != CMD) {
            continue;
        }
        fputs(n->data, stdout);
        fputs(" :", stdout);
        for (a = n->next; a != NULL && a->type != CMD; a = a->next) {
            if (a->type == SYMBOL && a->data[0] == '|') {
                break;
            }
            fputs(" ", stdout);
            fputs(a->data, stdout);
        }
        fputs("\n", stdout);
    }
}

//print every token and its type
void print_list(struct node *list) {
    struct node *n;
    int len;

    fputs("--------------\n", stdout);
    for (n = list; n != NULL; n = n->next) {
        fputs("Token: ", stdout);
        fputs(n->data, stdout);

        //pad
        for (len = strlen(n->data); len < 10; len++) {
            fputs(" ", stdout);
        }
        fputs(" Type: ", stdout);
        fputs(type_names[n->type], stdout);
        fputs("\n", stdout);
    }
}

//free every node and its string
void free_list(struct node *list) {
    struct node *next;

    while (list != NULL) {
        next = list->next;
        free(list->data);
        free(list);
        list = next;
    }
}

//one command in a pipeline
struct command {
    char *argv[64];  //args for execvp, NULL at the end
    char *in_file;   //file after <, or NULL
    char *out_file;  //file after >, or NULL
};

//take out quotes like a real shell does, "hello world" -> hello world
void strip_quotes(char *s) {
    char *out = s;
    char quote = 0; //quote we're inside of, 0 if none

    for (; *s != '\0'; s++) {
        if (quote == 0 && (*s == '"' || *s == '\'')) {
            quote = *s;
        } else if (*s == quote) {
            quote = 0;
        } else {
            *out++ = *s;
        }
    }
    *out = '\0';
}

//print a syntax error for the token where the line went wrong
void syntax_error(struct node *n) {
    fputs("syntax error near unexpected token `", stderr);
    if (n == NULL) {
        fputs("newline", stderr);
    } else {
        fputs(n->data, stderr);
    }
    fputs("'\n", stderr);
}

//turn the tokens up to the next & (or end of line) into commands
//moves *list past what was used, returns # of commands or -1 on an error
int build_commands(struct node **list, struct command *cmds, int *background) {
    struct node *n = *list;
    int count = 0; //commands finished so far
    int argc = 0;  //args in the current command

    *background = 0;
    cmds[0].in_file = NULL;
    cmds[0].out_file = NULL;

    while (n != NULL) {

        //< and > take the next token as the file name
        if (n->type == SYMBOL && (n->data[0] == '<' || n->data[0] == '>')) {
            if (n->next == NULL || n->next->type == SYMBOL) {
                syntax_error(n->next);
                return -1;
            }
            strip_quotes(n->next->data);
            if (n->data[0] == '<') {
                cmds[count].in_file = n->next->data;
            } else {
                cmds[count].out_file = n->next->data;
            }
            n = n->next->next;

        //| finishes this command and starts the next one
        } else if (n->type == SYMBOL && n->data[0] == '|') {
            if (argc == 0) {
                syntax_error(n);
                return -1;
            }
            if (count == 63) {
                fputs("too many commands\n", stderr);
                return -1;
            }
            cmds[count].argv[argc] = NULL;
            count++;
            argc = 0;
            cmds[count].in_file = NULL;
            cmds[count].out_file = NULL;
            n = n->next;

        //& ends this pipeline and runs it in the background
        } else if (n->type == SYMBOL && n->data[0] == '&') {
            if (argc == 0) {
                syntax_error(n);
                return -1;
            }
            *background = 1;
            n = n->next;
            break;

        //anything else is an arg for the current command
        } else {
            if (argc == 63) {
                fputs("too many arguments\n", stderr);
                return -1;
            }
            strip_quotes(n->data);
            cmds[count].argv[argc++] = n->data;
            n = n->next;
        }
    }

    //the last command can't be empty either ("ls |")
    if (argc == 0) {
        syntax_error(NULL);
        return -1;
    }
    cmds[count].argv[argc] = NULL;
    count++;

    *list = n;
    return count;
}

//run one pipeline, wait for it unless it's in the background
void execute(struct command *cmds, int ncmds, int background)
{
    pid_t pid;
    int status;
    pid_t pids[64]; //children to wait for
    int fds[2];     //pipe to the next command
    int prev = -1;  //read end of the pipe from the last command
    int fd;
    int i, started;

    for (i = 0; i < ncmds; i++) {

        //pipe to the next command, unless this is the last one
        if (i < ncmds - 1) {
            if (pipe(fds) < 0) {
                perror("pipe");
                break;
            }
        }
        fflush(stdout);

        pid = fork();
        if (pid < 0) { //failed fork
            perror("fork");
            if (i < ncmds - 1) {
                close(fds[0]);
                close(fds[1]);
            }
            break;
        }
        if (pid == 0) {
            //child

            //read from the last command's pipe, write into the next one
            if (prev != -1) {
                dup2(prev, STDIN_FILENO);
                close(prev);
            }
            if (i < ncmds - 1) {
                close(fds[0]);
                dup2(fds[1], STDOUT_FILENO);
                close(fds[1]);
            }

            //< and > swap stdin/stdout for a file
            if (cmds[i].in_file != NULL) {
                fd = open(cmds[i].in_file, O_RDONLY);
                if (fd < 0) {
                    perror(cmds[i].in_file);
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (cmds[i].out_file != NULL) {
                fd = open(cmds[i].out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    perror(cmds[i].out_file);
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            //execute command
            execvp(cmds[i].argv[0], cmds[i].argv);
            //failed exec
            if (errno == ENOENT) {
                fprintf(stderr, "%s: command not found\n", cmds[i].argv[0]);
            } else {
                perror(cmds[i].argv[0]);
            }
            exit(127);
        }

        //parent closes the pipe ends only the children use
        //(if they stay open, the next command never sees end of input)
        pids[i] = pid;
        if (prev != -1) {
            close(prev);
            prev = -1;
        }
        if (i < ncmds - 1) {
            close(fds[1]);
            prev = fds[0];
        }
    }

    //only open here if the loop stopped early
    if (prev != -1) {
        close(prev);
    }

    //wait for child exit, unless it's running in the background
    started = i;
    if (!background) {
        for (i = 0; i < started; i++) {
            if (waitpid(pids[i], &status, 0) < 0) {
                perror("waitpid");
            }
        }
    }
}

int main(void)
{
    char line[1024];         /* the input line                 */
    struct node *list;       //parsed line
    struct node *rest;       //part of the line not run yet
    struct command cmds[64]; //one pipeline
    int ncmds;
    int background;

    fputs("Type 'exit' to quit\n", stdout);
    fputs("Supports | < > &\n\n", stdout);

    while (1)
    {                                     /* repeat until done ....         */

        //clean up background jobs that finished, so they don't stay zombies
        while (waitpid(-1, NULL, WNOHANG) > 0) {
        }

        fputs("Shell -> ", stdout);       /*   display a prompt             */
        fflush(stdout); //show the prompt now so error messages come after it
        if (fgets(line, 1024, stdin) == NULL) { /* read in the command line */
            exit(0);                      /*   end of input: quit           */
        }
        list = parse(line);               /*   parse the line               */
        if (list == NULL) {               /*   blank line: prompt again     */
            continue;
        }
        if (strcmp(list->data, "exit") == 0) { /* is it an "exit"?     */
            free_list(list);
            exit(0);                      /*   exit if it is                */
        }

        //run each pipeline on the line (& splits them), then free it
        rest = list;
        while (rest != NULL) {
            ncmds = build_commands(&rest, cmds, &background);
            if (ncmds < 0) {
                break;
            }
            execute(cmds, ncmds, background);
        }
        free_list(list);
    }
    return 0;
}