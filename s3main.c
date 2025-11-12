#include "s3.h"

int main(int argc, char *argv[]){

    ///Stores the command line input
    char line[MAX_LINE];

    ///Stores pointers to command arguments.
    ///The first element of the array is the command name.
    char *args[MAX_ARGS];

    ///Stores the number of arguments
    int argsc;

    while (1) {

        read_command_line(line);


        if(command_with_pipe(line)){
            launch_pipe(line,args,&argsc);
        }

        else{
            launch_cmd(line,args,&argsc,1);
        }
        
    }

    return 0;
    
}
