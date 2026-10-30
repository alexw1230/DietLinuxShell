/*
 * ESE 333 Project 2 - Part 2: Reading Input and Parsing
 *
 * Reads a line and splits it into tokens: commands, one-dash flags,
 * two-dash flags, arguments, and the symbols | < > &. Each token goes into
 * its own node of a linked list along with its type. Nothing is executed
 * yet; the shell prints the list back to show the line was parsed correctly.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE    1024   /* size of the input buffer                      */
#define TOKEN_WIDTH 10     /* pad "Token: xxx" so the Type column lines up  */

/* The kinds of token a line can contain. */
typedef enum { CMD, ARG, FLAG1, FLAG2, SYMBOL } TokenType;

/* Printable names, in the same order as the enum above. */
static const char *type_names[] = { "CMD", "ARG", "FLAG1", "FLAG2", "SYMBOL" };

/* One node of the linked list: a token's text, its type, and the next node. */
typedef struct Token {
    char *text;
    TokenType type;
    struct Token *next;
} Token;

static int is_space(char c)  { return c == ' ' || c == '\t' || c == '\n'; }
static int is_symbol(char c) { return c == '|' || c == '<' || c == '>' || c == '&'; }

/* Allocate a node holding a copy of the len characters starting at start. */
Token *new_token(const char *start, size_t len, TokenType type)
{
    Token *t = malloc(sizeof(Token));
    if (t == NULL) { perror("malloc"); exit(1); }

    t->text = malloc(len + 1);                  /* +1 for the '\0' */
    if (t->text == NULL) { perror("malloc"); exit(1); }
    memcpy(t->text, start, len);
    t->text[len] = '\0';

    t->type = type;
    t->next = NULL;
    return t;
}

/*
 * Decide what kind of word this is, based on where it appears:
 *   - right after < or >  -> ARG   (it's a file name, even if it starts with -)
 *   - when a command is expected (start of line, after | or &) -> CMD
 *   - starts with "--"    -> FLAG2
 *   - starts with "-"     -> FLAG1 (a lone "-" counts as an ARG)
 *   - anything else       -> ARG
 */
TokenType classify_word(const char *word, int expect_cmd, int after_redirect)
{
    if (after_redirect)
        return ARG;
    if (expect_cmd)
        return CMD;
    if (word[0] == '-' && word[1] == '-')
        return FLAG2;
    if (word[0] == '-' && word[1] != '\0')
        return FLAG1;
    return ARG;
}

/*
 * Split a line into a linked list of tokens and return the head
 * (NULL for a blank line).
 *   - Spaces and tabs separate tokens.
 *   - | < > & are always tokens by themselves, with or without spaces
 *     around them, so "ls|grep x" gives ls, |, grep, x.
 *   - Text inside "double" or 'single' quotes stays in one token, quotes
 *     included, so echo "hello world" gives echo, "hello world".
 */
Token *tokenize(const char *line)
{
    Token *head = NULL, *tail = NULL, *t;
    int expect_cmd = 1;         /* is the next word a command name?   */
    int after_redirect = 0;     /* was the previous token < or >?     */
    const char *p = line;

    while (*p != '\0') {
        if (is_space(*p)) {                     /* skip whitespace */
            p++;
            continue;
        }

        if (is_symbol(*p)) {                    /* one-character symbol */
            t = new_token(p, 1, SYMBOL);
            if (*p == '|' || *p == '&')
                expect_cmd = 1;                 /* a new command follows   */
            after_redirect = (*p == '<' || *p == '>');
            p++;
        } else {                                /* a word: scan to its end */
            const char *start = p;
            while (*p != '\0' && !is_space(*p) && !is_symbol(*p)) {
                if (*p == '"' || *p == '\'') {
                    char quote = *p++;          /* jump to the closing quote */
                    while (*p != '\0' && *p != quote)
                        p++;
                    if (*p != '\0')
                        p++;                    /* step past closing quote   */
                } else {
                    p++;
                }
            }
            t = new_token(start, p - start, ARG);
            t->type = classify_word(t->text, expect_cmd, after_redirect);
            if (t->type == CMD)
                expect_cmd = 0;
            after_redirect = 0;
        }

        /* Append to the end of the list. */
        if (head == NULL)
            head = t;
        else
            tail->next = t;
        tail = t;
    }
    return head;
}

/* Free every node and the text it holds. */
void free_list(Token *head)
{
    while (head != NULL) {
        Token *next = head->next;
        free(head->text);
        free(head);
        head = next;
    }
}

/*
 * Print the command summary:
 *   Commands : ls grep
 *   ls : -l
 *   grep : .c
 * Each command's line lists everything after it up to the next | or the
 * next command, so redirections show up too: "cat : < input.txt > output.txt".
 */
void print_commands(Token *head)
{
    Token *t, *a;

    fputs("Commands :", stdout);
    for (t = head; t != NULL; t = t->next) {
        if (t->type == CMD) {
            fputs(" ", stdout);
            fputs(t->text, stdout);
        }
    }
    fputs("\n", stdout);

    for (t = head; t != NULL; t = t->next) {
        if (t->type != CMD)
            continue;
        fputs(t->text, stdout);
        fputs(" :", stdout);
        for (a = t->next; a != NULL && a->type != CMD; a = a->next) {
            if (a->type == SYMBOL && a->text[0] == '|')
                break;
            fputs(" ", stdout);
            fputs(a->text, stdout);
        }
        fputs("\n", stdout);
    }
}

/* Print every node: "Token: <text padded>  Type: <type>". */
void print_tokens(Token *head)
{
    Token *t;
    size_t len;

    fputs("--------------\n", stdout);
    for (t = head; t != NULL; t = t->next) {
        fputs("Token: ", stdout);
        fputs(t->text, stdout);
        for (len = strlen(t->text); len < TOKEN_WIDTH; len++)
            fputs(" ", stdout);                 /* pad to a fixed column */
        fputs(" Type: ", stdout);
        fputs(type_names[t->type], stdout);
        fputs("\n", stdout);
    }
}

int main(void)
{
    char line[MAX_LINE];                    /* the input line          */
    Token *list;                            /* the parsed tokens       */

    fputs("Type 'exit' to quit\n", stdout);
    fputs("Try commands with multiple arguments and spaces\n\n", stdout);

    while (1) {                             /* repeat until done ....  */
        fputs("Shell -> ", stdout);         /*   display a prompt      */
        fflush(stdout);                     /*   make sure it shows now */

        /* fgets returns NULL at end of input (Ctrl-D or end of a piped script). */
        if (fgets(line, sizeof(line), stdin) == NULL) {
            fputs("\n", stdout);
            break;
        }
        line[strcspn(line, "\n")] = '\0';   /*   drop the trailing newline */

        list = tokenize(line);              /*   build the linked list */

        if (list == NULL)                   /*   blank line: re-prompt */
            continue;
        if (strcmp(list->text, "exit") == 0) {
            free_list(list);
            break;
        }

        print_commands(list);               /*   show what was parsed  */
        print_tokens(list);
        free_list(list);                    /*   done with this line   */
    }
    return 0;
}