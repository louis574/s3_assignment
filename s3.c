#include "s3.h"

void construct_shell_prompt(char shell_prompt[])
{
    // get the current working directory

    char cwd[MAX_PATH];
    if (getcwd(cwd, sizeof(cwd)) == NULL){
        printf("Max prompt length exceded");
        return;
    };
    snprintf(shell_prompt, MAX_PROMPT_LEN, "[s3:%s]$ ", cwd);
}

///Prints a shell prompt and reads input from the user
void read_command_line(char line[])
{
    char shell_prompt[MAX_PROMPT_LEN];
    construct_shell_prompt(shell_prompt);
    printf("%s", shell_prompt);

    ///See man page of fgets(...)
    if (fgets(line, MAX_LINE, stdin) == NULL)
    {
        perror("fgets failed");
        exit(1);
    }
    ///Remove newline (enter)
    
    if(line[strlen(line)-1] = '\n'){
        line[strlen(line) - 1] = '\0';
    }
}



void my_parse_cmd(char line[], char *args[], int *argsc){
    char space = ' ';
    generic_tokeniser(line,space,args,argsc);
    for(int i = 0; i < *argsc;i++){
        args[i] = quote_remover(args[i]);
    }
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

void execute_program(char* args[], int argsc)
{

    child(args, argsc);

}

void launch_cmd(char line[],char* args[], int* argsc, int child){

    if(command_with_redirection(line)){
        
        
        
        my_parse_cmd(line, args, argsc);

        
        if(child){
            launch_program_with_redirection(args, *argsc);
        }
        else{
            execute_redirection(args, *argsc);
        }
        reap();            
    }

    else{

        my_parse_cmd(line, args, argsc);

        if(child){
            launch_program(args, *argsc); 
        }
        else{
            execute_program(args, *argsc);
        }

        reap();
    }
}

///////////////////////////////////////////////////////////////////
/////////////////////////// cd ////////////////////////////////////
///////////////////////////////////////////////////////////////////

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

    char *target = NULL;

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
        else if(args[1][1] == '/'){
            chdir(getenv("HOME"));
            char *new_path_pointer = &args[1][2]; 
            


            chdir(new_path_pointer);
                        
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
                printf("No such file or directory\n");
            };
    }
    
    if (chdir(target) == -2) {
        printf("error with final execution\n");
        return -1;
    }

    strncpy(lwd, cwd, MAX_PATH -1);
    lwd[MAX_PROMPT_LEN -1] = '\0';
    return 0;
}

void init_lwd(char lwd[]){
    char cwd[MAX_PATH];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        lwd[0] = '\0';
        return;
    }
    
    
    strncpy(lwd, cwd, MAX_PATH - 1);
    lwd[MAX_PROMPT_LEN - 1] = '\0';
}





//////////////////////////////////////////////////////////////////
/////////////////////// Pipes ////////////////////////////////////
//////////////////////////////////////////////////////////////////


int command_with_pipe(char line[]){
    int in_speech = 0;
    
    for(int i = 0; line[i] != '\0'; i++){
        
        char c = line[i];
        
        if(c == '"'){
            in_speech = !in_speech;
        }

        if( c=='|' && !in_speech){
            return 1;
        }
    }
    return 0;
}

void launch_pipe(char line[], char* args[], int* argsc){

    char* pipe_args[MAX_PIPE_LEN];
    int pipe_argsc;
    char pipe_char = '|';

    generic_tokeniser(line, pipe_char, pipe_args, &pipe_argsc);

    int new_pipe[2];
    int old_pipe[2];
    
    
    
    for(int i = 0; i < pipe_argsc; i++){
        if(i < pipe_argsc-1){
            pipe(new_pipe);
        }

        int rc = fork();

        if (rc < 0){
            fprintf(stderr, "fork failed\n");
        exit(1);
        }
        else if(rc == 0){


            if(i == 0){
                dup2(new_pipe[1],1);
                close(new_pipe[0]);
                close(new_pipe[1]);
            }

            else if(i == pipe_argsc - 1){
                dup2(old_pipe[0],0);
                close(old_pipe[0]);
                close(old_pipe[1]);
            }

            else{
                dup2(old_pipe[0],0);
                dup2(new_pipe[1],1);
                
                close(new_pipe[0]);
                close(new_pipe[1]);
                close(old_pipe[0]);
                close(old_pipe[1]);
            }

            //execute
            launch_cmd(pipe_args[i], args, argsc,0);
        
        
        }

        else{
            if(i>0){
                close(old_pipe[0]);
                close(old_pipe[1]);
            }

            if(i < pipe_argsc - 1){
                old_pipe[0] = new_pipe[0];
                old_pipe[1] = new_pipe[1];
            }
        }


    }

    while(wait(NULL) > 0);






}








///////////////////////////////////////////////////////////////
/////////////////// Redirects /////////////////////////////////
///////////////////////////////////////////////////////////////




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

void execute_redirection(char *args[], int argsc){
    char direction,operation;

    redirect_parse(args, argsc, &direction, &operation);

    char* child_instruction[argsc-1];
    for(int i = 0; i < argsc-2; i++){
        child_instruction[i] = args[i];
    }
    child_instruction[argsc-2] = NULL;

    char* file = args[argsc-1];


    if(direction == 'r'){
        child_with_input_redirected(child_instruction, file, operation);

    }
    else if(direction == 'w'){
        child_with_output_redirected(child_instruction, file, operation);

    }

    


}



void child_with_input_redirected(char *instruction[], char* file, char operation){
    int fd = open(file, O_RDONLY, 0666);
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
        fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }
    else if(operation == 'a'){
        fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0666);
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
    int in_speech = 0;
    
    for(int i = 0; line[i] != '\0'; i++){
        
        char c = line[i];
        
        if(c == '"'){
            in_speech = !in_speech;
        }

        if(( c=='>' | c=='<') && !in_speech){
            return 1;
        }
    }
    return 0;
}



///////////////////////////////////////////////////////////////////////
////////// Utility Functions - tokenising/parsing /////////////////////
///////////////////////////////////////////////////////////////////////

void generic_tokeniser(char line[], char parse_char, char* args[], int* argsc){
    int in_speech = 0;
    int string_start = 1;
    int i = 0;
    *argsc = 0;


    while(line[i] != '\0' && *argsc < MAX_ARGS-1){

        if(string_start && line[i] != parse_char){
            args[*argsc] = &line[i];
            string_start = 0;
            (*argsc)++;
        }



        if(line[i] == '"'){
            in_speech = !in_speech;
        }

        else if(!in_speech && line[i] == parse_char){
            line[i] = '\0';
            string_start = 1;
        }

    
        
        
        
        i++;
    
    }
    args[*argsc] = NULL;


    for(int x = 0; x<*argsc; x++){
        args[x] = whitespace_trim(args[x]);


    }

}


char* whitespace_trim(char* start){
    int i = 0;
    int j = strlen(start) - 1;


    while(start[i] == ' '){
        i++;
    }

    while(start[j] == ' ' && j >= i){
        start[j] = '\0';
        j--;
    }



    return &start[i];
}


char* quote_remover(char* string){
    if(string[0] == '"'){
        int len = strlen(string);
        string[len-1] = '\0';
        return &string[1];
    }
    return string;
}







/*   Old parse command

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
*/