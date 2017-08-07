#include "parser.h"

int token_type(char * token)
{
    if (!strcmp(token, "|"))
        return TOKEN_PIPE;
	if (!strcmp(token, "||"))
		return TOKEN_JOIN_FAIL;
	if (!strcmp(token, "&&"))
		return TOKEN_JOIN_SUCCESS;
	if (!strcmp(token, ";"))
		return TOKEN_JOIN_ANY;
	if (!strcmp(token, "<"))
		return TOKEN_IN_REDIRECT;
	if (!strcmp(token, ">"))
		return TOKEN_OUT_REDIRECT;
	if (!strcmp(token, ">>"))
		return TOKEN_OUT_APPEND;

	return TOKEN_OTHER;
}

char * token_type_str(char * token)
{
	switch(token_type(token)) {
	case TOKEN_PIPE: return STR(TOKEN_PIPE);
	case TOKEN_JOIN_FAIL: return STR(TOKEN_JOIN_FAIL);
	case TOKEN_JOIN_SUCCESS: return STR(TOKEN_JOIN_SUCCESS);
	case TOKEN_JOIN_ANY: return STR(TOKEN_JOIN_ANY);
	case TOKEN_IN_REDIRECT: return STR(TOKEN_IN_REDIRECT);
	case TOKEN_OUT_REDIRECT: return STR(TOKEN_OUT_REDIRECT);
	case TOKEN_OUT_APPEND: return STR(TOKEN_OUT_APPEND);
	case TOKEN_OTHER: return STR(TOKEN_OTHER);
	}
}

struct token * tokenize(char * str)
{
    char * token = NULL;
    char * delim = " ";
    struct token * token_list = NULL,
                 * last_token = NULL;

    while (token = strsep(&str, delim)) {
        LOG("token = '%s', type: %s, len: %lu\n", token, token_type_str(token), strlen(token));
		if(strlen(token) <= 0)
			continue;

        struct token * new_token = (struct token *) malloc(sizeof(struct token));

        if (!new_token) {
			fprintf(stderr, "allocation failed for new token\n");
			exit(1);
        }

        new_token->str = strdup(token);
        if (!new_token->str) {
			fprintf(stderr, "allocation failed in strdup");
			exit(1);
        }
        new_token->next = NULL;

        if (token_list == NULL) {
            token_list = new_token;
        } else {
            last_token->next = new_token;
        }

        last_token = new_token;
    }

    return token_list;
}

Command * build_commands(struct token * tokens)
{
    Command * commands = NULL,
			* last_command = NULL,
			* previous_command = NULL;
    struct token * token_ptr = tokens;
    int parse_state = STATE_NEED_NEW_COMMAND, parse_error = false;

    for (; token_ptr && !parse_error; token_ptr = token_ptr->next)
    {
    	struct token curr_token = *token_ptr;
		int type = token_type(curr_token.str);

        switch (parse_state)
        {
        case STATE_NEED_NEW_COMMAND:
            if (type == TOKEN_OTHER) {
            	Command * new_command = (Command *) malloc(sizeof(Command));

            	if(!new_command) {
					fprintf(stderr, "allocation failed for command\n");
					exit(1);
            	}

				new_command->cmd = strdup(curr_token.str);
				if (!new_command->cmd) {
					fprintf(stderr, "allocation failed in strdup");
					exit(1);
				}

				new_command->argv = NULL;
            	new_command->next = NULL;
            	new_command->arg_list = NULL;
            	new_command->last_arg = NULL;
            	new_command->input_file = NULL;
            	new_command->input_mode = O_NONE;
            	new_command->output_file = NULL;
            	new_command->output_mode = O_NONE;

				if(previous_command && previous_command->output_mode == O_PIPE) {
					new_command->input_mode = I_PIPE;
				}

            	if (commands == NULL) {
					commands = new_command;
            	} else {
					last_command->next = new_command;
            	}

				last_command = new_command;

				parse_state = STATE_NEED_ANY_TOKEN;
            } else {
            	LOG("error: invalid command\n");
            	parse_error = true;
            }
            break;
        case STATE_NEED_ANY_TOKEN:
        	switch (type)
        	{
			case TOKEN_IN_REDIRECT:
				last_command->input_mode = I_FILE;
				parse_state = STATE_NEED_IN_PATH;
				break;
			case TOKEN_OUT_REDIRECT:
				if(last_command->output_mode != O_NONE) {
					LOG("error: ambiguous redirect of output\n");
					parse_error = true;
					break;
				}
				last_command->output_mode = O_WRITE;
				parse_state = STATE_NEED_OUT_PATH;
				break;
			case TOKEN_OUT_APPEND:
				if(last_command->output_mode != O_NONE) {
					LOG("error: ambiguous redirect of output\n");
					parse_error = true;
					break;
				}
				last_command->output_mode = O_APPEND;
				parse_state = STATE_NEED_OUT_PATH;
				break;
			case TOKEN_JOIN_FAIL:
				last_command->next_command_exec_on = NEXT_ON_FAIL;
				previous_command = last_command;
				parse_state = STATE_NEED_NEW_COMMAND;
				break;
			case TOKEN_JOIN_SUCCESS:
				last_command->next_command_exec_on = NEXT_ON_SUCCESS;
				previous_command = last_command;
				parse_state = STATE_NEED_NEW_COMMAND;
				break;
			case TOKEN_JOIN_ANY:
				last_command->next_command_exec_on = NEXT_ON_ANY;
				previous_command = last_command;
				parse_state = STATE_NEED_NEW_COMMAND;
				break;
			case TOKEN_PIPE:
				if(last_command->output_mode != O_NONE) {
					LOG("error: ambiguous redirect of output\n");
					parse_error = true;
					break;
				}
				last_command->output_mode = O_PIPE;
				last_command->next_command_exec_on = NEXT_ON_ANY;
				previous_command = last_command;
				parse_state = STATE_NEED_NEW_COMMAND;
				break;
			case TOKEN_OTHER:
			{
				Arg * new_arg = (Arg *) malloc(sizeof(Arg));

				if (!new_arg) {
					fprintf(stderr, "allocation failed for new arg\n");
					exit(1);
				}

				new_arg->next = NULL;
				new_arg->arg = strdup(curr_token.str);
				if (!new_arg->arg) {
					fprintf(stderr, "allocation failed in strdup");
					exit(1);
				}

				if (last_command->arg_list == NULL) {
					last_command->arg_list = new_arg;
				} else {
					last_command->last_arg->next = new_arg;
				}
				last_command->last_arg = new_arg;
				parse_state = STATE_NEED_ANY_TOKEN;
				break;
			}
			default:
				LOG("error: unknown token type\n");
				parse_error = true;
        	}
            break;
        case STATE_NEED_IN_PATH:
        	if (type == TOKEN_OTHER) {
				last_command->input_file = strdup(curr_token.str);
				if (!last_command->input_file) {
					fprintf(stderr, "allocation failed in strdup");
					exit(1);
				}
				parse_state = STATE_NEED_ANY_TOKEN;
			} else {
				LOG("error: expected input path but found other token\n");
				parse_error = true;
			}
            break;
        case STATE_NEED_OUT_PATH:
        	if (type == TOKEN_OTHER) {
				last_command->output_file = strdup(curr_token.str);
				if (!last_command->output_file) {
					fprintf(stderr, "allocation failed in strdup");
					exit(1);
				}
				parse_state = STATE_NEED_ANY_TOKEN;
			} else {
				LOG("error: expected output path but found other token\n");
				parse_error = true;
			}
            break;
        }
    }

	if(parse_error)
		return NULL;

    return commands;
}

Command * parse(char * str)
{
    Command * commands = NULL;
    struct token * tokens = NULL, *tokens_temp = NULL;

    tokens = tokenize(str);

    commands = build_commands(tokens);

	free_tokens(tokens);

    return commands;
}

void print_command(Command * temp)
{
	Arg * args;

	if(!temp)
		return;

	args = temp->arg_list;

	LOG("cmd: %s\n", temp->cmd);

	LOG("args: ");
	if(!args)
		LOG("-");
	while(args) {
		LOG("%s ", args->arg);
		args = args->next;
	}
	LOG("\n");

	LOG("input file: %s\n", temp->input_file);

	LOG("input mode: ");
	switch(temp->input_mode)
	{
		case I_FILE: LOG("I_FILE\n"); break;
		case I_PIPE: LOG("I_PIPE\n"); break;
		default: LOG("-\n"); break;
	}

	LOG("output file: %s\n", temp->output_file);

	LOG("output mode: ");
	switch(temp->output_mode)
	{
		case O_APPEND: LOG("O_APPEND\n"); break;
		case O_PIPE: LOG("O_PIPE\n"); break;
		case O_WRITE: LOG("O_WRITE\n"); break;
		default: LOG("-\n"); break;
	}

	LOG("command combine mode: ");
	switch(temp->next_command_exec_on)
	{
		case NEXT_ON_ANY: LOG("NEXT_ON_ANY\n"); break;
		case NEXT_ON_FAIL: LOG("NEXT_ON_FAIL\n"); break;
		case NEXT_ON_SUCCESS: LOG("NEXT_ON_SUCCESS\n"); break;
		default: LOG("-\n"); break;
	}
}

void print_commands(Command * commands)
{
	Command * temp = commands;
	LOG("-----\n");
	while(temp) {
		print_command(temp);
		LOG("---\n");
		temp = temp->next;
	}
}

void free_tokens(struct token * tokens)
{
	struct token * temp = tokens;
	while(tokens) {
		tokens = tokens->next;
		free(temp->str);
		free(temp);
		temp = tokens;
	}
}

void free_commands(Command * commands)
{
	Command * temp = NULL;
	Arg * arg_temp = NULL;

	temp = commands;
	while(commands) {
		commands = commands->next;
		free(temp->cmd);
		free(temp->argv);
		free(temp->input_file);
		free(temp->output_file);

		arg_temp = temp->arg_list;
		while(temp->arg_list) {
			temp->arg_list = temp->arg_list->next;
			free(arg_temp->arg);
			free(arg_temp);
			arg_temp = temp->arg_list;
		}

		free(temp);
		temp = commands;
	}

}
