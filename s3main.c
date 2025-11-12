#include "s3.h"

int main(int argc, char *argv[]){

    ///Stores the command line input
    char line[MAX_LINE];

    ///The last (previous) working directory 
    char lwd[MAX_PROMPT_LEN]; 

    init_lwd(lwd);

    // the current working directory
    char cwd[MAX_PROMPT_LEN]

    ///Stores pointers to command arguments.
    ///The first element of the array is the command name.
    char *args[MAX_ARGS];
    


    ///Stores the number of arguments
    int argsc;

    while (1) {
        read_command_line(line, lwd);
        
        if (is_cd(line)){
            parse_command(line, args, &argsc);
            run_cd(args, argsc, lwd);
        }
        else if(command_with_redirection(line)){
            parse_command(line, args, &argsc);

            launch_program_with_redirection(args, argsc);
            
            reap();            
        }

        else{

            parse_command(line, args, &argsc);

            launch_program(args, argsc); 

            reap();
        }
    }

    return 0;
    
}
