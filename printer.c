#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>

int main(int argc, char* argv[]){
    if (argc != 2) {
        fprintf(stderr, "Wrong number of arguments!\n");
        exit(EXIT_FAILURE);
    }
    int input_desc;
    if((input_desc = open(argv[1], O_RDONLY, S_IWUSR | S_IRUSR)) < 0){
        fprintf(stderr, "File to be printed could not be opened\n");
        exit(EXIT_FAILURE);
    }
    char buf[BUFSIZ];
    int len;
    while ((len = read(input_desc, buf, BUFSIZ)) > 0)
        write(STDOUT_FILENO, buf, len);
    if (len < 0) {
        fprintf(stderr, "Reading process failed\n");
        exit(EXIT_FAILURE);
    }
    if (close(input_desc) == -1) {
        fprintf(stderr, "Input file %s could not closed\n", argv[1]);
        exit(EXIT_FAILURE);
    };
    exit(EXIT_SUCCESS);
}