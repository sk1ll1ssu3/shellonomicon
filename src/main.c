#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define DEFAULT_NUMBER_OF_TOKENS 64
#define EXIT_CMD "exit"
#define CHANGE_DIR_CMD "cd"
#define CHANGE_ENV_VAR_CMD "set"
#define REMOVE_ENV_VAR_CMD "unset"
#define BUILTIN_EXIT_STATUS -1
#define BUILTIN_CMD_STATUS 1
#define NOT_BUILTIN_CMD_STATUS 0

void shell(void);
char **parse(char *cmd);
static char **ensure_cap(char **args, int pos, int *cap);
int execute(char **args);
static int handle_builtin(char **args);
static int count_str_array_len(char **arr);

int main(void) {
  shell();
  return 0;
}

void shell(void) {
  char *cmd = NULL;
  int status = 1;
  size_t len = 0;
  while (status) {
    printf("> ");
    fflush(stdout);
    if (getline(&cmd, &len, stdin) == -1) { // EOF
      free(cmd);
      exit(EXIT_SUCCESS);
    }
    char **args = parse(cmd);
    if (args[0] == NULL) {
      free(args);
      continue;
    }
    status = execute(args);
    free(args);
  }
  free(cmd);
}

char **parse(char *cmd) {
  int pos = 0;
  int cap = DEFAULT_NUMBER_OF_TOKENS;
  char **args = malloc(cap * sizeof(char *));
  if (!args) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  const char *delims = " \t\n\r";
  for (char *token = strtok(cmd, delims); token; token = strtok(NULL, delims)) {
    args = ensure_cap(args, pos, &cap);
    args[pos++] = token;
  }
  args = ensure_cap(args, pos, &cap);
  args[pos] = NULL;
  return args;
}

static char **ensure_cap(char **args, int pos, int *cap) {
  if (pos >= *cap) {
    *cap *= 2;
    char **tmp_args = realloc(args, *cap * sizeof(char *));
    if (!tmp_args) {
      perror("realloc");
      free(args);
      exit(EXIT_FAILURE);
    }
    args = tmp_args;
  }
  return args;
}

int execute(char **args) {
  int builtins = handle_builtin(args);
  if (builtins)
    return (builtins == BUILTIN_EXIT_STATUS ? 0 : builtins);
  pid_t pid = fork();
  if (pid == 0) { // child process
    execvp(args[0], args);
    // why am I still here!?
    perror("execvp");
    _exit(EXIT_FAILURE);
  } else if (pid == -1) {
    perror("fork");
    return 1;
  } else { // I'm the parent
    int wstatus;
    do {
      waitpid(pid, &wstatus, 0);
    } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
  }
  return 1;
}

static int handle_builtin(char **args) {
  int args_len = count_str_array_len(args);
  if (strcmp(args[0], EXIT_CMD) == 0) {
    return BUILTIN_EXIT_STATUS;
  } else if (strcmp(args[0], CHANGE_DIR_CMD) == 0) {
    if (args_len <= 1) {
      fprintf(stderr, "shellonomicon: Expected path after cd command.\n");
    } else if (args_len > 2) {
      fprintf(stderr, "shellonomicon: Warning: Ignored extra parameters after "
                      "the path name.\n");
    } else {
      if (chdir(args[1]) == -1) {
        perror("shellonomicon: cd");
      }
    }
    return BUILTIN_CMD_STATUS;
  } else if (strcmp(args[0], CHANGE_ENV_VAR_CMD) == 0) {
    if (args_len < 3) {
      fprintf(stderr, "shellonomicon: Not enough number of arguments for "
                      "setting environment variables.\n");
    } else {
      if (setenv(args[1], args[2], 1) == -1) {
        perror("shellonomicon: set");
      }
    }
    return BUILTIN_CMD_STATUS;
  } else if (strcmp(args[0], REMOVE_ENV_VAR_CMD) == 0) {
    if (args_len < 2) {
      fprintf(stderr, "shellonomicon: Not enough number of arguments for "
                      "unsetting environment variables.\n");
    } else {
      if (unsetenv(args[1]) == -1) {
        perror("shellonomicon: unset");
      }
    }
    return BUILTIN_CMD_STATUS;
  }
  return NOT_BUILTIN_CMD_STATUS;
}

static int count_str_array_len(char **arr) {
  int len = 0;
  while (arr[len] != NULL) {
    ++len;
  }
  return len;
}
