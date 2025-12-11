#ifndef _S3_H_
#define _S3_H_

///See reference for what these libraries provide
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <glob.h>

///Constants for array sizes, defined for clarity and code readability
#define MAX_LINE 1024
#define MAX_ARGS 128
#define MAX_PROMPT_LEN 512
#define MAX_PIPE_LEN 50
#define MAX_PATH 256
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define MAX_BATCH_SIZE 32

///Enum for readable argument indices (use where required)
enum ArgIndex
{
    ARG_PROGNAME,
    ARG_1,
    ARG_2,
    ARG_3,
};

///With inline functions, the compiler replaces the function call 
///with the actual function code;
///inline improves speed and readability; meant for short functions (a few lines).
///the static here avoids linker errors from multiple definitions (needed with inline).
static inline void reap()
{
    wait(NULL);
}

typedef struct Node {
    char *data;         // string stored in the node
    struct Node *next;  // pointer to next node
    struct Node *prev;
} Node;


//for the current line before its entered.
typedef struct List {
    int size;
    int count;
    Node *head;
    char *currentline;
} LinkedStack;



///Shell I/O and related functions (add more as appropriate)
void read_command_line(char line[], LinkedStack* history);
void construct_shell_prompt(char shell_prompt[]);
void parse_command(char line[], char *args[], int *argsc);
int command_with_redirection(char line[]);

///Child functions (add more as appropriate)
void child(char *args[], int argsc);
void child_with_input_redirected(char *instruction[], char* file, char operation);
void child_with_output_redirected(char *instruction[], char* file, char operation);

///Program launching functions (add more as appropriate)
void launch_program(char *args[], int argsc);
void launch_program_with_redirection(char *args[], int argsc);

///cd

int is_cd(char line[]);
int run_cd(char *args[], int argsc, char lwd[]);
void init_lwd(char lwd[]);

void redirect_parse(char *args[], int argsc, char* direction, char* operation);
void generic_tokeniser(char line[], char parse_char, char* args[], int* argsc);
char* whitespace_trim(char* start);
void my_parse_cmd(char line[], char *args[], int *argsc);
char* quote_remover(char* string);
int command_with_pipe(char line[]);
void launch_cmd(char line[],char* args[], int* argsc, int child);
void launch_pipe(char line[], char* args[], int* argsc);
void execute_program(char* args[], int argsc);
void execute_redirection(char *args[], int argsc);
int command_with_batch(char line[]);
void launch_batch(char line[], char* args[], int* argsc, char* lwd);



int sub_shell_detect(char line[]);

char* sub_shell_trim(char line[]);




char*  sub_shell_split(char line[]);

void sub_shell_child(char line[],char** args, int argsc, char* lwd);


void sub_shell_handler(char line[], char** args, int* argsc, char* lwd);

void sub_shell_aware_batch_tokeniser(char line[], char* args[], int* argsc);



int glob_in_operand(char *in);


void expand_glob_in_params(char *args[], int *argsc);





#endif