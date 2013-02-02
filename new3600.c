
/**
 * CS3600, Spring 2013
 * Project 1 Starter Code
 * (c) 2013 Alan Mislove
 *
 * You should use this (very simple) starter code as a basis for 
 * building your shell.  Please see the project handout for more
 * details.
 */

#include "3600sh.h"

#define USE(x) (x) = (x)

//define max string lengths using same convention as strlen
//e.g. memory allocated = length + 1
#define MAX_DIR_LENGTH 100
#define MAX_CMD_LENGTH 100
#define MAX_HOSTNAME_LENGTH 100
#define MAX_INPUT_LENGTH 100
#define MAX_NUM_ARGS 10
#define EXIT_COMMAND "exit"
#define NUM_ARGS_STEP 5
#define CMD_WORD_CHUNK 10

typedef char bool;
#define TRUE 1
#define FALSE 0

typedef int fd; 
#define STDIN 0
#define STDOUT 1
#define STDERR 2

typedef char status;
#define INVALID_SYNTAX 1
#define INVALID_ESCAPE 2
#define FOREGROUND 4
#define EOF_FOUND 8

void printPrompt();

char** readArgs(status* error, fd* redirection);
void deleteArgs(char** args);
void memoryError();
void readCommand();
bool addWord(char* word, char** arguments, int* argCount, int* argSteps, status* error, fd* redirection);

int main(int argc, char*argv[]) {
  // Code which sets stdout to be unbuffered
  // This is necessary for testing; do not change these lines
  USE(argc);
  USE(argv);
  setvbuf(stdout, NULL, _IONBF, 0); 

  // Main loop that reads a command and executes it
  while (1) {         
    // You should issue the prompt here
    printPrompt();
    // You should read in the command and execute it here
    readCommand();
  }

  return 0;
}

void printPrompt() {
  char* username = getenv("USER");
  char* hostname = (char*)calloc(MAX_HOSTNAME_LENGTH + 1, sizeof(char));
  if (!hostname) {
    memoryError();
  }

  gethostname(hostname, MAX_HOSTNAME_LENGTH);
  *(hostname + MAX_HOSTNAME_LENGTH) = 0;
  char* directory = getcwd(NULL,MAX_DIR_LENGTH);
  printf("%s@%s:%s> ", username, hostname, directory);
  free(hostname);
}

void readCommand() {
	char  **file = calloc(3, sizeof(char*));
  	if (!file) {
  		memoryError();
  	}

	for (int i = 0; i < 3; i++)
	{
	    file[i] = calloc(MAX_INPUT_LENGTH + 1, sizeof(char));
	    if (!file[i]) {
	      	memoryError();
	    }
	}

	status error_handle = 0;
	fd redirection = 0;
	char** args = readArgs(&error_handle, &redirection); 
	bool terminate = EOF_FOUND & error_handle;

	switch(error_handle & (INVALID_SYNTAX | INVALID_ESCAPE)){
		case INVALID_SYNTAX:
			printf("Error: Invalid syntax.\n");
			return;
		case INVALID_ESCAPE:
			printf("Error: Unrecognized escape sequence.\n");
			return;
	}

	if (args[0] && args[1] == NULL && strncmp(args[0], EXIT_COMMAND, strlen(EXIT_COMMAND)) == 0) {
		do_exit();
	}
  
 	int restore_stdout = 0;
	int restore_stdin = 0; 
	int restore_stderr = 0; 
 	int f = -1;
	

	//TODO fix this
	if (file[STDOUT][0])//type==(2<<STDOUT))
	{ 
		f = open(file[STDOUT],O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR); 
		/*Open a file with the path "file" and the intention to read and write from it. */
		/*If it does not exist create it and give read and write permissions to the user*/
        	restore_stdout = dup(STDOUT); //keep a copy of STDOUT
        	close(STDOUT); //Close STDOUT
		dup(f); //Copy the same file descriptor but set it to the newly closed STDOUT
		close(f); //Close original file
	}
	if (file[STDIN][0])//type==(2<<STDIN))
	{ 
		f = open(file[STDIN],O_RDONLY); 
        	restore_stdin = dup(STDIN); 
        	close(STDIN); 
		dup(f); 
		close(f); 
	}
	if (file[STDERR][0])//type==(2<<STDERR))
	{
		f = open(file[STDERR],O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR); 
        	restore_stderr = dup(STDERR); //keep a copy of STDOUT
        	close(STDERR); //Close STDOUT
		dup(f); //Copy the same file descriptor but set it to the newly closed STDOUT
		close(f); //Close original file
	}


  if(strcmp(args[0],"cd") == 0)
  {
     int error = 0; 
     if(args[1])
     	  error = chdir(args[1]);
     else
	error = chdir("/home");
     if(error)
	printf("cd: %s: No such file or directory\n",args[1]);
	
	return;
  }
  
  pid_t parent = fork();
  
  if (parent < 0) {
    printf("we fucked up");  
    do_exit();
  }
  else if (!parent) {
    if (execvp(args[0], args)) {
    	if((*args)[0]) {
          printf("Error: ");
          switch(errno) {
            case 1: printf("Permission denied.\n"); break;
            case 2: printf("Command not found.\n"); break;
            default: printf("Unkown error.\n"); break;
    	    }
      }
    }
    exit(0);
  } else {
    waitpid(parent, NULL, 0);
    if (terminate) {
      do_exit();
    }
  }
  if(restore_stdin)
  {
  	close(STDIN);
  	dup(restore_stdin);
  	close(restore_stdin);
  }
  if(restore_stdout)
  {
  	close(STDOUT);
  	dup(restore_stdout);
  	close(restore_stdout);
  }
  if(restore_stderr)
  {
  	close(STDERR);
  	dup(restore_stderr);
  	close(restore_stderr);
  }

  for (int i = 0; i < 3; i++) {
    free(file[i]);
  }
  free(file);
  deleteArgs(args);
  free(args);
}


char** readArgs(status* error, fd* redirection) {
	char** arguments = (char**)calloc(NUM_ARGS_STEP, sizeof(char*));
	if (!arguments) {
		memoryError();
	}
	int argCount = 0;
	int argSteps = 1;
	char c = getchar(); 
	char* cmdWord = (char*)calloc(CMD_WORD_CHUNK + 1, sizeof(char));
	if (!cmdWord) {
		memoryError();
	}
	int wordChunks = 1;
	int wordLength = 0;

	while (c != '\n' && c != EOF) {
		if (c == '\t' || c == ' ') {
			while (c == '\t' || c == ' ') {
				c = getchar();
			}
			cmdWord[wordLength] = 0;
			addWord(cmdWord, arguments, &argCount, &argSteps, error, redirection);
			wordChunks = 1;
			wordLength = 0;
			cmdWord = (char*)calloc(CMD_WORD_CHUNK + 1, sizeof(char));
			if (!cmdWord) {
				memoryError();
			}
			continue;
		}
		if (c == '\\') {
			c = getchar();
			switch(c) {
				case '\\':
				case ' ':
				case '&':
					break;
				case 't':
					c = '\t';
					break;
				default:
					*error |= INVALID_ESCAPE;
					return NULL;
			}
			cmdWord[wordLength] = c;
			wordLength++;
			c = getchar();
			continue;
		}
		if (wordLength == CMD_WORD_CHUNK * wordChunks) {
			wordChunks++;
			char* newWord = (char*)calloc(CMD_WORD_CHUNK * wordChunks + 1, sizeof(char));
			if (!newWord) {
				memoryError();
			}
			memcpy(newWord, cmdWord, sizeof(char) * CMD_WORD_CHUNK * (wordChunks - 1));
			free(cmdWord);
			cmdWord = newWord;
		}
		cmdWord[wordLength] = c;
		wordLength++;
		c = getchar();
	}
	if (wordLength) {
		cmdWord[wordLength] = 0;
		addWord(cmdWord, arguments, &argCount, &argSteps, error, redirection);
	}
	if (c == EOF) {
		(*error) |= EOF_FOUND;
	}
	return arguments;
}

bool addWord(char* word, char** arguments, int* argCount, int* argSteps, status* error, fd* redirection) {
	if (strncmp(word, "<", 1) == 0) {
		*redirection = STDIN;
	} else if (strncmp(word, ">", 1) == 0) {
		*redirection = STDOUT;
	} else if (strncmp(word, "2>", 2) == 0) {
		*redirection = STDERR;
	} else {
		if (*argCount == (*argSteps * NUM_ARGS_STEP)) {
			(*argSteps)++;
			char** newArguments = (char**)calloc(*argSteps * NUM_ARGS_STEP, sizeof(char*));
			if (!newArguments) {
				memoryError();
			}
			memcpy(newArguments, arguments, sizeof(char**) * ((*argSteps) - 1));
			free(arguments);
			arguments = newArguments;
		}
		arguments[*argCount] = word;
		(*argCount)++;
	}
	return TRUE;
}

void deleteArgs(char** args) {
  char* arg = *(args);
  while (arg != 0) {
    free(arg);
    arg = *(++args);
  }
}

void memoryError() {
  printf("Error: A memory oops has occurred.");
  do_exit();
}

// Function which exits, printing the necessary message
//
void do_exit() {
  printf("So long and thanks for all the fish!\n");
  exit(0);
}