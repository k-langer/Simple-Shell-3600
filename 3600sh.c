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

typedef char bool;
#define TRUE 1
#define FALSE 0

typedef int fd; 
#define STDIN 0
#define STDOUT 1
#define STDERR 2
void printPrompt();
void readCommand();
bool buildInput(char* input,char** file,int* type);
//bool buildInput(char* input);
char** parseArgs(char* input);
void deleteArgs(char** args);
void memoryError();

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
  //char* directory = getenv("PWD");
  printf("%s@%s:%s> ", username, hostname, directory);
  free(hostname);
}

void readCommand() {
  char* command = calloc(MAX_INPUT_LENGTH + 1, sizeof(char));
  if (!command) {
    memoryError();
  }
  char  **file = calloc(3, sizeof(char*));
  if (!file) 
  	memoryError();
  for (int i = 0; i < 3; i++)
  {
	file[i] = calloc(MAX_INPUT_LENGTH + 1, sizeof(char));
	 if (!file[i]) 
  		memoryError();
  }
  int type = 0;

  bool terminate = buildInput(command, file, &type);
  if (strncmp(command, EXIT_COMMAND, strlen(EXIT_COMMAND)) == 0) {
    do_exit();
  }

  //char** args = parseArgs(command,&pipein,&pipeout);
    char** args = parseArgs(command); 
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
	if((*args)[0])//!terminate)
	{
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

  for (int i = 0; i < 3; i++)
	free(file[i]);
  free(file);
  deleteArgs(args);
  free(args);
  free(command);
}

bool buildInput(char* input, char** file, int* type) {

  int length = 0;
  
  char c = getchar();
  char prev = c;
  bool file_mode = FALSE;
  int index;
  while (c != '\n' && c != EOF && length <= MAX_INPUT_LENGTH) {
  if (file_mode)
  {
	int file_len = 0;
	while (c != '<' && c != '>' && c != '\n' && c != EOF && file_len <= MAX_INPUT_LENGTH){
		if (c !=  ' ')
		{
			*(file[index]+file_len) = c;
			file_len++;
		}
	 	c = getchar();
	}
	if ( c != '<' && c != '>')
		break;
  }
  file_mode = FALSE;
  index = 0;
 //printf("HERE: %c\n",c);
   if(c == '>')
   {
	if (prev == '2')
	{
		*(input + length) = 0;
		length--;
  	 	index = STDERR;
		*type += 4;
	}
	else 
	{
		index = STDOUT;
		*type += 2;
	}
	file_mode = TRUE;
	
  }
   else if(c == '<')
   {
	index = STDIN;
	*type += 1;
	file_mode = TRUE;
  }
  else if(prev != c || c != ' ' )
  {
    *(input + length) = c;
    length++;
   }
 
   prev = c;	
   c = getchar();
   
  }
  if(*(input + length-1) == ' ')
	length--;
  *(input + length) = 0;
  if (c == EOF) {
    return TRUE;
  } else {
    return FALSE;
  }
}
char** parseArgs(char* input) {
  char** arguments = (char**)calloc(MAX_NUM_ARGS, sizeof(char*));
  if (!arguments) {
    memoryError();
  }
  int argCount = 0;
  char c = *input;
  char* arg = (char*)calloc(MAX_CMD_LENGTH, sizeof(char));
  if (!arg) {
    memoryError();
  }
  int argLength = 0;
  bool esc_mode = FALSE;
  while (c != 0) {
  
   
    esc_mode = FALSE;
    if (c == '\\')
    {
	input++;
	c = *input;
	if (c == '\\' || c == '>' || c == '<' ||  c == '&' || c == ' ')
	{
		esc_mode = TRUE;
	}
	else if (c =='n' || c == 't')
	{
		input++;
		c=*input;
	}
	else 
	{
		printf("invalid escape character\n");
		//TODO find better way to handle this error
		//deleteArgs(arguments);
		//free(arguments);
		//arguments = (char**)calloc(1, sizeof(char*));
		//arguments[0]= (char*)calloc(1, sizeof(char));
		//return arguments;
	}
		
    }
 
    if ((c == ' ' || c == '\t') && !esc_mode) {
      if (argLength) {
        *(arg + argLength) = 0;
        *(arguments + argCount) = arg;
        argCount++;
        arg = (char*)calloc(MAX_CMD_LENGTH, sizeof(char));
        if (!arg) {
          memoryError();
        }
        argLength = 0;
	
      }  
    } else {
      *(arg + argLength) = c;
      argLength++;
    }
    c = *(++input);
  }
   
  *(arg + argLength) = 0;
  *(arguments + argCount) = arg;
  argCount++;
  *(arguments + argCount) = NULL;
  return arguments;
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
