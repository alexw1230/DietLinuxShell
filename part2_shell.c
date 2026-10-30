#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

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

//same as pt 1, not called here tho
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
        if (errno == ENOENT) {
            fprintf(stderr, "%s: command not found\n", argv[0]);
        } else {
            perror(argv[0]);
        }
        exit(127);
    }

    //wait for child exit
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
    }
}

int main(void)
{
    char line[1024];         /* the input line                 */
    struct node *list;       //parsed line

    fputs("Type 'exit' to quit\n", stdout);
    fputs("Try commands with multiple arguments and spaces\n\n", stdout);

    while (1)
    {                                     /* repeat until done ....         */
        fputs("Shell -> ", stdout);       /*   display a prompt             */
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

        //print what was parsed, then free it
        print_commands(list);
        print_list(list);
        free_list(list);
    }
    return 0;
}