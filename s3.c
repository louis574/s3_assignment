#include "s3.h"

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[])
{
    // get the current working directory

    char cwd[MAX_PATH];
    if (getcwd(cwd, sizeof(cwd)) == NULL){
        printf("Max prompt length exceded");
        return;
    };
    snprintf(shell_prompt, MAX_PROMPT_LEN, "[S3:%s]$", cwd);
}

///Prints a shell prompt and reads input from the user
void read_command_line(char line[])
{
    char shell_prompt[MAX_PROMPT_LEN];

    construct_shell_prompt(shell_prompt);
    
    printf("%s", shell_prompt);
    fflush(stdout);

    ///See man page of fgets(...)
    if (fgets(line, MAX_LINE, stdin) == NULL)
    {
        perror("fgets failed");
        exit(1);
    }
    ///Remove newline (enter)
    line[strlen(line) - 1] = '\0';
}

void parse_command(char line[], char *args[], int *argsc)
{
    ///Implements simple tokenization (space delimited)
    ///Note: strtok puts '\0' (null) characters within the existing storage, 
    ///to split it into logical cstrings.
    ///There is no dynamic allocation.

    ///See the man page of strtok(...)
    char *token = strtok(line, " ");
    *argsc = 0;
    while (token != NULL && *argsc < MAX_ARGS - 1)
    {
        args[(*argsc)++] = token;
        token = strtok(NULL, " ");
    }
    
    args[*argsc] = NULL; ///args must be null terminated
}

///Launch related functions
void child(char* args[], int argsc)
{
    ///Implement this function:

    if(execvp(args[0],args) == -1){
        fprintf(stderr, "execvp launch failed\n");
        exit(1);
    }


    ///Use execvp to load the binary 
    ///of the command specified in args[ARG_PROGNAME].
    ///For reference, see the code in lecture 3.
}

void launch_program(char* args[], int argsc)
{
    ///Implement this function:
    if(argsc == 1 && strcmp(args[0],"exit") == 0){
        exit(0);
    }

    ///fork() a child process.
    ///In the child part of the code,
    ///call child(args, argv)
    ///For reference, see the code in lecture 2.
    int rc = fork();
    if (rc < 0){
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (rc == 0){ //child
        child(args, argsc);
    }
    else{ //parent
        wait(NULL);
    }

    ///Handle the 'exit' command;
    ///so that the shell, not the child process,
    ///exits.

} 


//thats fire
void launch_program_with_redirection(char *args[], int argsc){
    char direction,operation;

    redirect_parse(args, argsc, &direction, &operation);

    char* child_instruction[argsc-1];
    for(int i = 0; i < argsc-2; i++){
        child_instruction[i] = args[i];
    }
    child_instruction[argsc-2] = NULL;

    char* file = args[argsc-1];


    int rc = fork();
    if (rc < 0){
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (rc == 0){ //child
        if(direction == 'r'){
            child_with_input_redirected(child_instruction, file, operation);

        }
        else if(direction == 'w'){
            child_with_output_redirected(child_instruction, file, operation);

        }
    }
    else{ //parent
        wait(NULL);
    }

    


}

void child_with_input_redirected(char *instruction[], char* file, char operation){
    int fd = open(file, O_RDONLY);
    if(fd < 0){
        fprintf(stderr,"Error opening file");
        exit(1);
    }

    dup2(fd, STDIN_FILENO);

    close(fd); //to make sure the file gets closed after process

    if(execvp(instruction[0],instruction) == -1){
        fprintf(stderr, "execvp launch failed\n");
        exit(1);
    }

}

void child_with_output_redirected(char *instruction[], char* file, char operation){
    int fd;
    
    if(operation == 'o'){
        fd = open(file, O_WRONLY | O_CREAT);
    }
    else if(operation == 'a'){
        fd = open(file, O_WRONLY | O_CREAT | O_APPEND);
    }
    if(fd < 0){
        fprintf(stderr,"Error opening file");
        exit(1);
    }

    dup2(fd, STDOUT_FILENO);

    close(fd); //to make sure the file gets closed after process

    if(execvp(instruction[0],instruction) == -1){
        fprintf(stderr, "execvp launch failed\n");
        exit(1);
    }

}

void redirect_parse(char *args[], int argsc, char* direction, char* operation){
    char *redirect = args[argsc-2];
    if(redirect[0] == '<'){
        *direction = 'r'; // 'r' for read from file
    }
    else{
        *direction = 'w'; // 'w' for write to file
    }
    
    if(redirect[1] == '\0'){
        *operation = 'o'; //'o' for overwrite
    }
    else{
        *operation = 'a'; //'a' for append
    }
}



int command_with_redirection(char line[]){
    for(int i = 0; line[i] != '\0'; i++){
        char c = line[i];
        if( c=='>' | c=='<'){
            return 1;
        }
    }
    return 0;
}

// requirements 
// distinguish between the arguments of cd 
// no argument -> users home 
// - -> change to previous directory , will require a last directory variable
//use getcwd to print the current directory the user is in
// maintain the current directory in construct shell prompt

int is_cd(char line[]){
    if (line[0] == 'c' && line[1] == 'd'){
        return 1;
    }
    return 0;
}

// different cases 
// cd with no args or with ~ - $home
// cd - assign pwd to cwd
// cd <path> - go to that path 

int run_cd(char *args[], int argsc, char lwd[]){
    // work out what type of cd command this is 
    char cwd[MAX_PATH];

    const char *target = NULL;

    if (!getcwd(cwd, sizeof(cwd))) {
        perror("getcwd");
        return -1;
    }

    if (args[1] == NULL){
            target = getenv("HOME");
        }
    else if (args[1][0] == '~'){
        if (args[1][1] == '\0'){
            target = getenv("HOME");
        }
        else {
            chdir(getenv("HOME"));
                        
        }
    }
    else if (strcmp(args[1],"-") == 0) {
        if (lwd[0] == '\0'){
            printf("no previous directory");
            return -1;
        }    
        target = lwd;
        }
    else {
            if (chdir(args[1])){
                printf("Path not recognized");
            };
    }
    
    if (chdir(target) == -1) {
        printf("error with final execution");
        return -1;
    }

    strncpy(lwd, cwd, MAX_PATH -1);
    lwd[sizeof(lwd) - 1] = '\0';
    return 0;
}

void init_lwd(char lwd[]){
    char cwd[MAX_PATH];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        lwd[0] = '\0';
        return;
    }
    
    
    strncpy(lwd, cwd, MAX_PATH - 1);
    lwd[sizeof(lwd) - 1] = '\0';
}