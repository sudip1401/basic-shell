#include "executor.h"

char ** build_argv (char * cmd, Arg * arg_list)
{
	int argc = 0, i = 0;
	Arg * arg = arg_list;
	char ** argv;

	argc = 1;
	while (arg) {
		argc++;
		arg = arg->next;
	}

	argv = (char **) malloc((argc + 1) * sizeof(char *));
	if (!argv) {
		fprintf(stderr, "memory allocation failed");
		exit(1);
	}

	argv[0] = cmd;

	arg = arg_list;
	for (i = 1; i < argc; i++) {
		argv[i] = arg->arg;
		arg = arg->next;
	}

	argv[argc] = NULL;

	LOG("argc = %d\n", argc);
	for (i = 0; i <= argc; i++) {
		LOG("argv[%d] = %s\n", i, argv[i]);
	}

	return argv;
}

int execute (Command * commands)
{
	Command *command = commands, *wait_from = NULL;
	pid_t child_pid;
	int child_read_from, child_write_to;
	int child_status, status = 0;
	int pipefd[2];
	char buf;

	command = commands;
	wait_from = NULL;

	while (command) {
		switch (command->input_mode) {
		case I_FILE:
			child_read_from = open(command->input_file, O_RDONLY);
			if (child_read_from == -1) {
				int err = errno;
				printf("unable to open file: '%s'\n", command->input_file);
				return err;
			}
			break;
		case I_PIPE:
			child_read_from = pipefd[0];
			break;
		case I_NONE:
		default:
			child_read_from = STDIN_FILENO;
			break;
		}

		switch (command->output_mode) {
		case O_WRITE:
			child_write_to = open(command->output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (child_write_to == -1) {
				printf("unable to open file: '%s'\n", command->output_file);
				return errno;
			}
			LOG("[%u] O_WRITE: parent opened file '%s' in write mode with desc: %d \n", getpid(), command->output_file, child_write_to);
			break;
		case O_APPEND:
			child_write_to = open(command->output_file, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (child_write_to == -1) {
				int err = errno;
				printf("unable to open file: '%s'\n", command->output_file);
				return err;
			}
			if (lseek(child_write_to, 0, SEEK_END) == -1) {
				int err = errno;
				printf("unable to seek");
				return err;
			}
			LOG("[%u] O_APPEND: parent opened file '%s' in append mode with desc: %d \n", getpid(), command->output_file, child_write_to);
			break;
		case O_PIPE:
			if (pipe(pipefd) != 0) {
				fprintf(stderr, "failed to create pipe");
				exit(1);
			}
			child_write_to = pipefd[1];
			LOG("[%u] O_PIPE: parent created a pipe with rd: %d, wd: %d\n", getpid(), pipefd[0], pipefd[1]);
			break;
		case O_NONE:
		default:
			child_write_to = STDOUT_FILENO;
		}

		command->argv = build_argv(command->cmd, command->arg_list);

		child_pid = fork();

		if (child_pid == -1) {
			LOG("fork failed");
			exit(1);
		}

		if (child_pid == 0) {
			LOG("[%u] child process started, will execute: %s\n", getpid(), command->cmd);

			if (child_read_from != STDIN_FILENO) {
				LOG("[%u] child closing stdin and dupping %d\n", getpid(), child_read_from);
				if (close(STDIN_FILENO) != 0) {
					fprintf(stderr, "[%u] error on closing stdin\n", getpid());
					exit(1);
				}
				if (dup(child_read_from) == -1) {
					fprintf(stderr, "dup failed");
					exit(1);
				}
				if (close(child_read_from) != 0) {
					fprintf(stderr, "[%u] error on closing read descriptor %d\n", getpid(), child_read_from);
					exit(1);
				}
			}

			if (child_write_to != STDOUT_FILENO) {
				LOG("[%u] child closing stdout and dupping %d\n", getpid(), child_write_to);
				if (close(STDOUT_FILENO) != 0) {
					fprintf(stderr, "[%u] error on closing stdout\n", getpid());
					exit(1);
				}
				if (dup(child_write_to) == -1) {
					fprintf(stderr, "dup failed");
					exit(1);
				}
				if (close(child_write_to) != 0) {
					fprintf(stderr, "[%u] error on closing write descriptor %d\n", getpid(), child_write_to);
					exit(1);
				}
			}

			if (execvp(command->cmd, command->argv) == -1) {
				fprintf(stderr, "exec failed\n");
				exit(1);
			}
		} else {
			command->pid = child_pid;

			if (wait_from == NULL) {
				wait_from = command;
			}

			if (command->output_mode != O_NONE) {
				LOG("[%u] parent closing write descriptor %d\n", getpid(), child_write_to);
				close(child_write_to);
			}

			if (command->input_mode != I_NONE) {
				LOG("[%u] parent closing read descriptor %d\n", getpid(), child_read_from);
				close(child_read_from);
			}

			if (command->output_mode != O_PIPE) {
				Command * temp = wait_from;
				status = 0;
				while (temp) {
					LOG("[%u] parent waiting for child with pid: %u\n", getpid(), temp->pid);
					do {
						waitpid(temp->pid, &child_status, 0);
					} while (!WIFEXITED(child_status) && !WIFSIGNALED(child_status));

					if (WIFEXITED(child_status)) {
						LOG("[%u] child with pid %u exited with status: %d\n", getpid(), temp->pid, WEXITSTATUS(child_status));
						status |= WEXITSTATUS(child_status);
					}

					if (WIFSIGNALED(child_status)) {
						LOG("[%u] child with pid %u was signaled: %d\n", getpid(), temp->pid, WTERMSIG(child_status));
						status |= WTERMSIG(child_status);
					}

					// wait till the current command's child process
					if (temp == command) {
						break;
					}
					temp = temp->next;
				}
				wait_from = NULL;
			}
		}

		switch (command->next_command_exec_on)
		{
		case NEXT_ON_SUCCESS:
			if (status == 0)
				command = command->next;
			else
				return status;
			break;
		case NEXT_ON_FAIL:
			if (status != 0)
				command = command->next;
			else
				return status;
			break;
		default:
		case NEXT_ON_ANY:
			command = command->next;
			break;
		}
	}

	return status;
}
