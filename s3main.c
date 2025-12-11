#include "s3.h"

int main(int argc, char *argv[]){

    ///Stores the command line input
    char line[MAX_LINE];

    ///Stores pointers to command arguments.
    ///The first element of the array is the command name.
    char *args[MAX_ARGS];

    ///Stores the number of arguments
    int argsc;

    ///The last (previous) working directory 
    char lwd[MAX_PROMPT_LEN]; 

    init_lwd(lwd);

    LinkedStack history = {0, 0, NULL, malloc(sizeof(char*))};



    while (1) {
        history.count = 0;
        memset(line, 0, sizeof(line));
        read_command_line(line, &history);


        if(command_with_batch(line) && !sub_shell_detect(line)){
            launch_batch(line,args, &argsc, lwd);

        }

        else{
        
            sub_shell_handler(line,args, &argsc, lwd);
        }
        
    }

    return 0;
    
}
