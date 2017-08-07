#include "shell.h"

int verbose = false;

void shell_run()
{
	int run;
	char * line = NULL;
    size_t buffer_size = 0;
	Command * commands = NULL;

    while(1)
	{
        printf("osh>");
        line = NULL;
        if (getline(&line, &buffer_size, stdin) != -1) {
			line[strlen(line) - 1] = '\0'; // remove the trailing '\n'
        } else {
			fprintf(stderr, "failed to read line");
            free(line);
            exit(1);
        }

        if (!strcmp(line, "exit")) {
			free(line);
            break;
        }

        LOG("line = '%s'\n", line);

        commands = parse(line);
        print_commands(commands);
		execute(commands);
		free_commands(commands);
		free(line);
    }
}

int main(int argc, char ** argv)
{
	if(argc > 1) {
		int i = 0;
		for(i = 1; i < argc; i++) {
			if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "verbose")) {
				verbose = true;
			}
		}
	} else {
		printf("Usage: run with a -v or verbose flag to see detailed output\n");
	}

	shell_run();
    return 0;
}

