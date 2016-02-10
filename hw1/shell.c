#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "tokenizer.h"

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
  {cmd_cd, "cd", "change working directory"},
  {cmd_pwd, "pwd", "print working directory"}
};

/* Prints a helpful description for the given command */
int cmd_help(struct tokens *tokens) {
  for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(struct tokens *tokens) {
  exit(0);
}

/* Changes the current working directory to the specified directory */
int cmd_cd(struct tokens *tokens) {
	char* newDir = tokens_get_token(tokens, 1);
	if (chdir(newDir) == 0) {
		return 1;
	}  else {
		fprintf(stdout, "cd: %s: No such file or directory\n", newDir);
		return -1;
	}


}

/* Prints the current working directory to standard output */
int cmd_pwd(struct tokens *tokens) {
  char wd[1024];
  if (getcwd(wd, sizeof(wd)) != NULL) {
    fprintf(stdout, "%s\n", wd);
    return 1;
  } else {
    return -1;
  }

}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* runs a program where the path is not specified */
void run_program(struct tokens *tokens, int redirect, int redirect_index) {
	char* prog = tokens_get_token(tokens, 0);

	char* env_path = getenv("PATH");
	char* copy_path = malloc(1024 * sizeof(char));
	strcpy(copy_path, env_path);

	size_t num_args;
	
	int status;
	int fildes;
	int temp_fildes;

	// open file and parse arguments correctly if redirect is needed
	if (redirect == 1 || redirect == 2) {
		num_args = redirect_index;

		char* filename = tokens_get_token(tokens, redirect_index + 1);
		fildes = open(filename, O_CREAT|O_RDWR, 0644);

		if (redirect == 1) {
			temp_fildes = dup(STDOUT_FILENO);
			dup2(fildes , STDOUT_FILENO);
		} else if (redirect == 2) {
			temp_fildes = dup(STDIN_FILENO);
			dup2(fildes , STDIN_FILENO);
		}
	} else {
		num_args = tokens_get_length(tokens);
	}
	
	char* arguments[num_args];
	char* full_prog_path = malloc(1024 * sizeof(char));
	char* directory = strtok(copy_path, ":");

	while (directory != NULL) {
		sprintf(full_prog_path, "%s/%s", directory, prog);

		// check if the program exists in that directory
		if (access(full_prog_path, F_OK) != -1) {
		// runs the program
			pid_t process_id = fork();

			if (process_id == 0) {
			// run program in child process
				for (int i = 0; i <= num_args; i++) {
					if (i == num_args) {
						arguments[i] = NULL;
					} else {
						arguments[i] = tokens_get_token(tokens, i);
					}
				}
				execv(full_prog_path, arguments);
				break;
			} else {	
			//parent

				wait(&status);
				break;
			}
		} else {
			directory = strtok(NULL, ":");
		}
	}

	if (redirect == 1) {
		dup2(temp_fildes, STDOUT_FILENO);
	} else if (redirect == 2) {
		dup2(temp_fildes, STDIN_FILENO);
	}

	free(copy_path);
	free(full_prog_path);    
}

/* runs a program where the path is specified */
void run_program_path(struct tokens *tokens, int redirect, int redirect_index) {
	size_t num_args;
	int status;
	int fildes;
	int temp_fildes;

	// open file and parse arguments correctly if redirect is needed
	if (redirect == 1 || redirect == 2) {
		num_args = redirect_index;

		char* filename = tokens_get_token(tokens, redirect_index + 1);
		fildes = open(filename, O_CREAT|O_RDWR, 0644);

		if (redirect == 1) {
			temp_fildes = dup(STDOUT_FILENO);
			dup2(fildes , STDOUT_FILENO);
		} else if (redirect == 2) {
			temp_fildes = dup(STDIN_FILENO);
			dup2(fildes , STDIN_FILENO);
		}
	} else {
		num_args = tokens_get_length(tokens);
	}

	char* arguments[num_args];

	char* prog = tokens_get_token(tokens, 0);
	pid_t process_id = fork();


	if (process_id == 0) {
		for (int i = 0; i <= num_args; i++) {
			if (i == num_args) {
				arguments[i] = NULL;
			} else {
				arguments[i] = tokens_get_token(tokens, i);
			}
		}
		execv(prog, arguments);
	} else {	
		wait(&status);
	}

	if (redirect == 1) {
		dup2(temp_fildes, STDOUT_FILENO);
	} else if (redirect == 2) {
		dup2(temp_fildes, STDIN_FILENO);
	}

}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

int main(int argc, char *argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      /* REPLACE this to run commands as programs. */
      // fprintf(stdout, "This shell doesn't know how to run programs.\n");
    	char* prog = tokens_get_token(tokens, 0);

    	int redirect = 0;
    	int num_args = tokens_get_length(tokens);
    	
    	int redirect_index = 0;

    	for (int i = 0; i < num_args; i++) {
    		if (strcmp(tokens_get_token(tokens, i), ">") == 0) {
	    		redirect = 1;
	    		redirect_index = i;
	    		break;
	    	} else if (strcmp(tokens_get_token(tokens, i), "<") == 0) {
	    		redirect = 2;
	    		redirect_index = i;
	    		break;
	    	}
    	}
	    	
    	int has_path = 0;
    	if (strchr(prog, '/') != NULL) {
    		has_path = 1;
    	}

    	if (!has_path) {
    		run_program(tokens, redirect, redirect_index);
    	} else {
    		run_program_path(tokens, redirect, redirect_index);
    	}
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
