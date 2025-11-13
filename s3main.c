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

    while (1) {

        read_command_line(line);


        if(command_with_batch(line)){
            launch_batch(line,args, &argsc, lwd);

        }

        else{
        

            if(strcmp(line,"exit") == 0){
                exit(0);
            }

            if (is_cd(line)){
                my_parse_cmd(line, args, &argsc);
                if (run_cd(args, argsc, lwd)  == -1){
                    printf("error in compiling cd function");

                };
            }


            else if(command_with_pipe(line)){
                launch_pipe(line,args,&argsc);
            }

            else{
                launch_cmd(line,args,&argsc,1);
            }
        }
        
    }

    return 0;
    
}
