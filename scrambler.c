#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

int main(int argc, char* argv[]){
    if (argc != 4) {
        printf("Wrong number of arguments!\n");
        exit(EXIT_FAILURE);
    }
    int output_desc;
    FILE * orig, * key;
    char command[BUFSIZ];
    sprintf(command, "./%s %s", "printer", argv[1]);
    orig = popen(command, "r");
    sprintf(command, "./%s %s", "printer", argv[2]);
    key = popen(command, "r");
    if (key == NULL || orig == NULL) {
        printf("No command is responding");
        exit(EXIT_FAILURE);
    }
    size_t biba, boba; //professional naming
    char * biba_buf, * boba_buf;
    biba_buf = NULL;
    if ((output_desc = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR)) < 0) {
        printf("Output file could not be opened\n");
        exit(EXIT_FAILURE);
    }
    size_t off = 0;
    do {
        off += BUFSIZ;
        biba_buf = (char *)realloc((void *)biba_buf, off);
    } while ((biba = fread(biba_buf + off - BUFSIZ, 1, BUFSIZ, key)) == BUFSIZ);
    if (!feof(key)) {
        close(output_desc);
        printf("Reading error\n");
        exit(EXIT_FAILURE);
    }
    off += biba - BUFSIZ;
    boba_buf = (char *)malloc(off);
    while ((boba = fread(boba_buf, 1, off, orig)) > 0) {
        for (int i = 0; i < boba; ++i) boba_buf[i] ^= biba_buf[i];
        if (write(output_desc, boba_buf, boba) < 0) {
            pclose(key);
            pclose(orig);
            close(output_desc);
            printf("Writing error\n");
            exit(EXIT_FAILURE);
        }
    }
    free(boba_buf);
    free(biba_buf);
    if (!feof(orig)) {
        close(output_desc);
        printf("Reading error\n");
        exit(EXIT_FAILURE);
    }
    pclose(key);
    pclose(orig);
    exit(EXIT_SUCCESS);
}