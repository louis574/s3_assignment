#include "s3.h"

int main(int argc, char *argv[]){

    ///Stores the command line input
    char line[MAX_LINE];

    ///Stores pointers to command arguments.
    ///The first element of the array is the command name.
    char *args[MAX_ARGS];

    ///Stores the number of arguments
    int argsc;

    char parse_char = '|';

    while (1) {
        read_command_line(line);

        generic_tokeniser(line, parse_char, args, &argsc);

        int i = 0;
        while(args[i] != NULL){

            printf("'%s'\n", args[i]);



            i++;
        }
    }

    return 0;

    
}
