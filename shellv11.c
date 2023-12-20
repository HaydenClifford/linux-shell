/**
 * My submitted version of the shell program that simulates a linux shell.
 *
 * Problems:
 * 1. Piping does not work as intended, it simply prints a message
 * saying the pipe was successful.
 * 2. Output redirection works as intended but only can happen once per line of input
 * 3. Input redirection does not work at all except for one time when I was testing 
 * and I was never able to replicate it with seemingly the exact same code.
 * 4. When compiling with -Wall there is a warning about passing an integer into fgets()
 * in my input_redirection() function
 *
 * Author: Hayden Clifford
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

void welcome();
void cd(char * );
void run_command(char**,int);
bool check_redirection(char ** , int);
void input_redirection(char ** , int);
bool check_pipe(char ** , int);
void make_pipe(char ** , int);
void output_redirect(char**,int,bool);

int main() {

  // Display welcome message on startup:
  welcome();

  char * argv[100];
  int buffer = 1024;
  char line[buffer];
  int argc;

  // Continue printing prompt and looking for input until program ends
  while (1) {
    printf("\n#)-> ");
    fgets(line, buffer, stdin);

    char * token;
    token = strtok(line, " \n");
    int i = 0;

    // Restart loop if no input or only ampersand
    if (token == NULL || strcmp(token, "&") == 0) {
      continue;
    }

    // Loop through and tokenize command line input
    while (token != NULL) {
      argv[i] = token;
      token = strtok(NULL, " \n");
      i++;
    }
    argc = i;
    argv[argc] = NULL;
    

    run_command(argv,argc);
  }
}

// Function to get the command from array of strings and then run it with arguments
void run_command(char* argv[], int argc) {
  bool amp=false;
  if (strcmp(argv[0], "exit") == 0) {
    exit(0);
  } else if (strcmp(argv[0], "cd") == 0) {
    if (argc<2) {
      argv[1]="~";
    }
    cd(argv[1]);
    return;
  } else if (strcmp(argv[argc - 1], "&") == 0) {
      argv[argc - 1] = NULL;
      argc--;
      amp=true;
  }
  
  // This function handles redirection and skips the rest if it finds it
  if (check_redirection(argv,argc)) {
    return;
  }
  
  // This function handles piping and skips the rest if it finds a pipe
  if (check_pipe(argv,argc)) {
    return;
  }
  
  // Create the child process and wait if there is no ampersand
  pid_t pid = fork();
  int status;
  if (pid < 0) {
    printf("Error\n");
    exit(1);
  } else if (pid == 0) {
    if (execvp(argv[0], argv) < 0) {
      printf("Error: invalid command\n");
      exit(1);
    }
  }
  if(!amp) {
    waitpid(pid, &status, 0);
  }
}

// Function to display the welcome message when shell is started
void welcome() {
  printf("Welcome to the Clifford Shell.\n\n");
  printf("Created by Hayden Clifford\n");
  printf("-------------------\n\n");
  printf("Quote of the day: \"Don't be afraid of anything.\" - me\n");
}

// Helper function to change directory when user inputs 'cd'
void cd(char * dir) {
  if (strcmp(dir, "~") == 0) {
    chdir(getenv("HOME"));
  } else {
    if (chdir(dir) == -1) {
      printf("Error: directory '%s' not found.\n", dir);
    }
  }
}

// Check if there is a redirection operator and send arguments to function
// Returns true if there is redirection, false if no redirection
bool check_redirection(char * argv[], int argc) {
  // Loop through arguments to find '<', '>', or '>>'
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '<') {
      input_redirection(argv, i);
      return true;
    } else if (argv[i][0] == '>') {
      if (argv[i][1] == '>') {            // If two arrows, append to file
        argv[i] = NULL;
        output_redirect(argv, i, false);
      } else {                            // If only one arrow, truncate file
        argv[i] = NULL;
        output_redirect(argv, i, true);
      }
      return true;
    }
  }
  return false;
}

// Checks if there is a pipe in the input
bool check_pipe(char * argv[], int argc) {
  // Loop through arguments to find '|'
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '|') {
      argv[i] = NULL;
      make_pipe(argv, i);
      return true;
    }
  }
  return false;
}

// Could not figure this part out
void make_pipe(char * argv[], int index) {
  printf("Pipe successful\n");
  /*
      int     fd[2];
          pid_t   pid;

          pipe(fd);
          
          if((pid = fork()) < 0) {
                  printf("Error\n");
                  exit(1);
          } else if (pid == 0) {
              dup2(fd[0], 1);
                  close(fd[0]);
                  
                  exit(0);
          } else {
                  close(fd[1]);

          }
          
          return(0);
   */
}

// Function to handle input redirection ('<')
// Does not work correctly and is very messy
void input_redirection(char * args[], int index) {
  char * argv[100];
  int argc = 0;
  int buffer = 1024;
  char line[buffer];
  printf("Redirecting input\n");
  
  pid_t pid = fork();
  int status;
  if (pid < 0) {
    printf("Error\n");
    exit(1);
  } else if (pid == 0) {
    int infile;
    if ((infile = open(args[index + 1], O_RDONLY)) < 0) {
      printf("Error\n");
      exit(1);
    }
    dup2(infile, 0);
    close(infile);
    
    fgets(line, buffer, infile);
    
    char * token;
    token = strtok(line, " \n");
    int i = 0;

    // Restart loop if no input or only ampersand
    if (token == NULL || strcmp(token, "&") == 0) {
      printf("Token was null\n");
      return;
    }

    // Loop through and tokenize command line input
    while (token != NULL) {
      argv[i] = token;
      token = strtok(NULL, " \n");
      i++;
    }
    argc = i;
    argv[argc] = NULL;
    
    printf("The command was %s\n",argv[0]);
    
    run_command(argv,argc);
  }
  waitpid(pid, &status, 0);
}

// Redirects output of command to file
// Appends or overwrites depending on number of arrows
void output_redirect(char* argv[], int index, bool trunc) {
  pid_t pid = fork();
  int status;
  
  if (pid < 0) {
    printf("Error\n");
    exit(1);
  } else if (pid == 0) {
    int outfile;
    if (trunc) {
      if ((outfile = open(argv[index + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
        printf("Error\n");
        exit(1);
      }
    } else {
      if ((outfile = open(argv[index + 1], O_WRONLY | O_CREAT | O_APPEND, 0666)) < 0) {
        printf("Error\n");
        exit(1);
      }
    }
    dup2(outfile, 1);
    close(outfile);
    if (execvp(argv[0], argv) < 0) {
      printf("Error: invalid command\n");
      exit(1);
    }
  }
  waitpid(pid, &status, 0);
}


