#include "s3.h"
#include <termios.h>
#include <unistd.h>


///////////////// command history ////////////////////////////


//raw mode
void set_raw_mode(int enable){
    static struct termios oldt, newt;

    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;

        newt.c_lflag &= ~(ICANON | ECHO);   
        newt.c_cc[VMIN]  = 1;              
        newt.c_cc[VTIME] = 0;              

        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
}



//Create a new node with a copy of the given string
Node *create_node(const char *str){
    Node *node = malloc(sizeof(Node));
    node->data = strdup(str);   
    node->next = NULL;
    return node;
}


//append node at the end of the list
void append(LinkedStack *list, const char *str){

    if (list->head == NULL) {
        list->head = create_node(str);
        list->size++;
        return;
    }

    Node *node = malloc(sizeof(Node));
    node->data = strdup(str);   // allocates and copies string
    node->next = list->head;
    list->head = node;
    list ->size ++;
}

// delete line from terminal
void delete_line(char line[], int len){
    for (int i = 0; i < len; i++) {
        printf("\b \b");
    }    
    fflush(stdout);
}

//Print line to terminal
void print_line(char line[], int len){
    for (int i = 0; i < len; i++) {
        putchar(line[i]);
    }
    fflush(stdout);
}


//returns pointer to end node in stack
Node* getnode(LinkedStack* history){
    Node *cur = history->head;
    int steps = 1;

    while (cur && cur->next && steps < history->count) {
        cur = cur->next;
        steps++;
    }

    return cur;
}

void read_command_line(char line[], LinkedStack* history)
{
    char shell_prompt[MAX_PROMPT_LEN];
    construct_shell_prompt(shell_prompt);
    printf("%s", shell_prompt);
    fflush(stdout);

    history -> count = 0;
    set_raw_mode(1);

    int c;
    int pos = 0;
    int newline = 0;
    while (newline == 0) {
        c = getchar();

        if (c == '\n' || c == '\r') {
            putchar('\n');
            newline = 1;
        } else if (c == 0x7f || c == '\b') {
            if (pos > 0) {
                pos--;
                line[pos] = '\0';
                printf("\b \b");
                fflush(stdout);
            }
        } else if (c == 0x1B){
            // ESC sequence
            int c2 = getchar();
            if (c2 == '['){
                int c3 = getchar();
                if (history->head) {
                    if (c3 == 'A'){//up arrow
                    
                        if (history -> count == 0 ){
                            strcpy(history->currentline, line);
                        }
                        if (history->size > history->count)
                            history -> count ++;
                        Node *cur = getnode(history);
                        
                        delete_line(line, pos);
                        strcpy (line, cur->data);
                        pos = strlen(line);
                        line[pos] = '\0';
                        print_line(line, pos);
                    } else if (c3 == 'B'){   //down arrow                    
                        
                        if (history -> count == 1 ){
                            delete_line(line, pos);
                            strcpy(line, history -> currentline);
                            history -> count --;
                            pos = strlen(line);
                            line[pos] = '\0';
                            print_line(line,pos);
                        }
                        else if (history -> count > 1 ){
                            history -> count --;
                            Node *cur = getnode(history);
                            delete_line(line, pos);
                            strcpy (line, cur->data);
                            pos = strlen(line);
                            line[pos] = '\0';
                            print_line(line, pos);
                    }
                    }
            }
            }
        } else {
            //normal character
            if (pos < MAX_LINE - 1){
                line[pos++] = (char)c;
                line[pos] = '\0';
                putchar(c);     
                fflush(stdout);
            }
        }
    }

    set_raw_mode(0);

    //terminate the string safely with null
    line[pos] = '\0';

    //store in history
    append(history, line);
}



/////////////////////////////////////////////////////////////

void construct_shell_prompt(char shell_prompt[])
{
    //get current working directory

    char cwd[MAX_PATH];
    if (getcwd(cwd, sizeof(cwd)) == NULL){
        printf("Max prompt length exceded");
        return;
    };
    snprintf(shell_prompt, MAX_PROMPT_LEN, "[s3:%s]$ ", cwd);
}




//parses command into an array
void my_parse_cmd(char line[], char *args[], int *argsc){
    char space = ' ';
    generic_tokeniser(line,space,args,argsc);
}

//launch functions
void child(char* args[], int argsc)
{
    //globbing
    //expands any params with * and then calls execvp
    expand_glob_in_params(args,&argsc);
    for(int i = 0; i < argsc;i++){
        args[i] = quote_remover(args[i]);
    }
    int i = 0;
    while(args[i] != NULL){
        i++;
    }
    if(execvp(args[0],args) == -1){
        fprintf(stderr, "execvp launch failed\n");
        exit(1);
    }
}

void launch_program(char* args[], int argsc)
{
    //exit command
    if(argsc == 1 && strcmp(args[0],"exit") == 0){
        exit(0);
    }

    //forking child process
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

}

void execute_program(char* args[], int argsc)
{

    child(args, argsc);

}


//launches normal and redirected commands
void launch_cmd(char line[],char* args[], int* argsc, int child){

    if(command_with_redirection(line)){
        
        
        
        my_parse_cmd(line, args, argsc);

        
        if(child){
            launch_program_with_redirection(args, *argsc);
        }
        else{
            execute_redirection(args, *argsc);
        }          
    }

    else{

        my_parse_cmd(line, args, argsc);

        int i = 0;

        while(args[i] != NULL){
            i++;

        }

        if(child){
            launch_program(args, *argsc); 
        }
        else{
            execute_program(args, *argsc);
        }
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


//check if cd command
int is_cd(char line[]){
    if (line[0] == 'c' && line[1] == 'd' && line[2] == ' '){
        return 1;
    }
    return 0;
}

// different cases 
// cd with no args or with ~ - $home
// cd - assign pwd to cwd
// cd <path> - go to that path 



//executes cd command
int run_cd(char *args[], int argsc, char lwd[]){
    // work out what type of cd command this is 
    char cwd[MAX_PATH];

    char *target = NULL;


    expand_glob_in_params(args, &argsc);

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
    //return to previous
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


//sets up lwd buffer
void init_lwd(char lwd[]){
    char cwd[MAX_PATH];

    if (getcwd(cwd, sizeof(cwd)) == NULL){
        lwd[0] = '\0';
        return;
    }
    
    
    strncpy(lwd, cwd, MAX_PATH - 1);
    lwd[MAX_PROMPT_LEN - 1] = '\0';
}





//////////////////////////////////////////////////////////////////
/////////////////////// Pipes ////////////////////////////////////
//////////////////////////////////////////////////////////////////

//detects if command has pipes - ignores if in ""
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


//parses, sets up pipes, and launches instructions for pipes
void launch_pipe(char line[], char* args[], int* argsc){

    char* pipe_args[MAX_PIPE_LEN];
    int pipe_argsc;
    char pipe_char = '|';

    generic_tokeniser(line, pipe_char, pipe_args, &pipe_argsc);

    int new_pipe[2];
    int old_pipe[2];
    
    
    //loop that opens pipes and passes relevent read write ends into each child
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



//launches redirection
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

    int argsc = 0;

    while(instruction[argsc]!=NULL){
        argsc++;
    }



    expand_glob_in_params(instruction,&argsc);
    for(int i = 0; i < argsc;i++){
        instruction[i] = quote_remover(instruction[i]);
    }
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

    int argsc = 0;

    while(instruction[argsc]!=NULL){
        argsc++;
    }


    expand_glob_in_params(instruction,&argsc);

    for(int i = 0; i < argsc;i++){
        instruction[i] = quote_remover(instruction[i]);
    }



    if(execvp(instruction[0],instruction) == -1){
        fprintf(stderr, "execvp launch failed\n");
        exit(1);
    }

}

//parses reditect, making direction and whether it isappend or overwrite version
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


//detects if command with redirection, ignores in in ""
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
///////////////////////// Batching ////////////////////////////////////
///////////////////////////////////////////////////////////////////////

//detects if command is batched, ignores if in ""
int command_with_batch(char line[]){
    int in_speech = 0;
    int in_sub_shell = 0;
    
    for(int i = 0; line[i] != '\0'; i++){
        
        char c = line[i];
        
        if(c == '"'){
            in_speech = !in_speech;
        }

        else if(c=='('){
            in_sub_shell = 1;
        }

        else if(c==')'){
            in_sub_shell = 0;
        }

        if( c==';' && !in_speech && !in_sub_shell){
            return 1;
        }
    }
    return 0;
}


void launch_batch(char line[], char* args[], int* argsc, char* lwd){

    printf("launching this batcj: %s\n", line);
    
    char* instructions[MAX_BATCH_SIZE];
    int instruction_argsc;


    sub_shell_aware_batch_tokeniser(line, instructions, &instruction_argsc);

    int i = 0;

    while(instructions[i] != NULL){

        printf("instruction going into sub shell handler from batched commands: %s\n", instructions[i]);

        sub_shell_handler(instructions[i], args, argsc, lwd);




        i++;
    }






}

////////////////////////////////////////////////////////////////////////
//////////////////////////////// subsheel //////////////////////////////////
//////////////////////////////////////////////////////////////////////////




//detects if current command is subshell
int sub_shell_detect(char line[]){
    line = whitespace_trim(&line[0]);

    int in_shell = 0;

    int i = 0;

    while(line[i] != '\0'){
        if(line[i] == '('){
            in_shell++;
        }
        else if(line[i] == ')'){
            in_shell--;
        }
        else if(line[i] ==';' && in_shell == 0){
            return 0;
        }


        i++;
    }



    if(line[0] == '(' && line[strlen(line)-1] == ')'){        
        return 1;
    }
    else{
        return 0;
    }
}


//trims whitespace
char* sub_shell_trim(char line[]){
    line = whitespace_trim(&line[0]);
    line[strlen(line)-1] = '\0';
    return whitespace_trim(&line[1]);
}



//splits up the sub_shell into cd and instructions (which can be batched)
char*  sub_shell_split(char line[]){

    printf("splitting: %s\n", line);
    
    if(line[0] == 'c' && line[1] == 'd' && line[2] == ' ' ){
        int i = 3;
        while(line[i] != '\0' && line[i] != ';'){
            i++;
        }
        if(line[i] == '\0'){
            printf("incorrect input format\n");
        }
        else{
            line[i] = '\0';
            return &line[i+1];
        }
    }
    else{
        printf("incorrect input format\n");
        printf("caused by: %s\n", line);
        exit(1);
    }
}


//calls subshell split and changes directory, they calls subshell handler on instructions
//to fascilitate execution of nested subshells
void sub_shell_child(char line[],char** args, int argsc, char* lwd){
    line = sub_shell_trim(line);
    char* cd = &line[0];
    char* ins = sub_shell_split(line);
    char tmp_cwd[MAX_PATH];
    getcwd(tmp_cwd, sizeof(tmp_cwd));
    my_parse_cmd(cd, args, &argsc);
    if (run_cd(args, argsc, lwd)  == -1){
        printf("error in compiling cd function");
        exit(1);
    }


    sub_shell_handler(ins,args, &argsc, lwd);


    exit(0);





}

//looks for subshell and sends it to appropriate handler function
//if not sends to batch handler for it to handle
void sub_shell_handler(char line[], char** args, int* argsc, char* lwd){



    if(sub_shell_detect(line)){
        int rc = fork();
        if (rc < 0){
            fprintf(stderr, "fork failed\n");
            exit(1);
        }
        else if (rc == 0){ //child
            sub_shell_child(line, args, *argsc, lwd);
        }
        else{ //parent
            wait(NULL);
        }
        
        
        }
    else{
        if(command_with_batch(line)){
            launch_batch(line,args, argsc, lwd);

        }

        else{
            

            if(strcmp(line,"exit") == 0){
                exit(0);
            }



            if (is_cd(line)){
                my_parse_cmd(line, args, argsc);
                if (run_cd(args, *argsc, lwd)  == -1){
                    printf("error in compiling cd function");

                };
            }


            else if(command_with_pipe(line)){
                launch_pipe(line,args,argsc);
            }

            else{
                launch_cmd(line,args,argsc,1);
            }

        }
    }
    
}


///////////////////////////////////////////////////////////////////////
////////// Utility Functions - tokenising/parsing /////////////////////
///////////////////////////////////////////////////////////////////////


//tokeniser we developed to enhance parsing so that characters witin "" would be ignored
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


//tokenises batched commands but if one of those is a subshell is doesnt tokenise on the semi-colons
//within that subshell, crucial for correct implementation of batched subshells
void sub_shell_aware_batch_tokeniser(char line[], char* args[], int* argsc){
    int in_speech = 0;
    int in_sub_shell = 0;
    int string_start = 1;
    int i = 0;
    *argsc = 0;

    char parse_char = ';';


    while(line[i] != '\0' && *argsc < MAX_ARGS-1){

        if(string_start && line[i] != parse_char){
            args[*argsc] = &line[i];
            string_start = 0;
            (*argsc)++;
        }



        if(line[i] == '"'){
            in_speech = !in_speech;
        }
        else if(line[i] == '('){
            in_sub_shell = 1;
        }
        else if(line[i] == ')'){
            in_sub_shell = 0;
        }

        else if(!in_sub_shell && !in_speech && line[i] == parse_char){
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

//clips whitespace from start/end on user inputs, used a lot


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


//removes quotes from around an operand, allows user to input a whole string into a function and it be 'one word'
char* quote_remover(char* string){
    if(string[0] == '"'){
        int len = strlen(string);
        string[len-1] = '\0';
        return &string[1];
    }
    return string;
}



////////////////////////////////////////////////////
////////////////////// globbing ///////////////////
//////////////////////////////////////////////////
//checks if globbing is in command
int glob_in_operand(char *in){
    int i = 0;
    int in_speech = 0;
    while(in[i] != '\0'){
        if(in[i] == '"'){
            i = !i;
        }
        else if((in[i] == '*' || in[i] == '?')&& !in_speech){
            return 1;
        }
        i++;
    }
    return 0;
}

//exbands glob, returns error if multiple
void expand_glob_in_params(char *args[], int *argsc) {
    char *expanded[MAX_ARGS];
    int newc = 0;
    int cd_ins = 0;
    if(strcmp(args[0],"cd")==0){
        cd_ins = 1;
    }

    for (int i = 0; i < *argsc; i++){
        char *operand = args[i];

        if (glob_in_operand(operand)){
            glob_t unpacked;

            int ret = glob(operand, 0, NULL, &unpacked);

            if (ret == 0){

                for (size_t j = 0; j < unpacked.gl_pathc; j++) {
                    expanded[newc++] = strdup(unpacked.gl_pathv[j]);
                }
            } else{

                expanded[newc++] = operand;
            }

            globfree(&unpacked);
        }
        else {
            expanded[newc++] = operand;
        }
    }

    if(cd_ins && newc != 2){
        printf("ERROR invalid input - incorrect input\n");
    }


    //Copy back
    else{
        for (int i = 0; i < newc; i++)
            args[i] = expanded[i];

        args[newc] = NULL;
        *argsc = newc;
    }
}