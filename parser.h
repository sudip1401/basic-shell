#ifndef PARSER_H
#define PARSER_H

#include "defs.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

// Parse state
#define STATE_NEED_NEW_COMMAND	1
#define STATE_NEED_IN_PATH		2
#define STATE_NEED_OUT_PATH		3
#define STATE_NEED_ANY_TOKEN	4

// Token type
#define TOKEN_JOIN_SUCCESS  1   // &&
#define TOKEN_JOIN_FAIL		2   // ||
#define TOKEN_PIPE			3	// |
#define TOKEN_IN_REDIRECT	4	// <
#define TOKEN_OUT_REDIRECT	5	// >
#define TOKEN_OUT_APPEND	6	// >>
#define TOKEN_JOIN_ANY		7	// ;
#define TOKEN_OTHER         8   // command/argument

struct token {
    char * str;
    struct token * next;
};

int token_type(char * token);
struct token * tokenize(char * str);
Command * parse(char *);
Command * build_commands(struct token * tokens);

void print_command(Command *);
void print_commands(Command *);

void free_commands(Command *);
void free_tokens(struct token *);

#endif // PARSER_H
