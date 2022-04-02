#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

// проверка выделения памяти
void check_mem(void * ptr){
    if (ptr == NULL) {
        perror("Memory problem");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]){
    if (argc != 4) {
        printf("Wrong number of arguments!\n");
        exit(EXIT_FAILURE);
    }
    char command[BUFSIZ];
    getcwd(command, BUFSIZ);
    strcat(command, "/printer");
    int id = fork(), pipe4orig_desc[2], pipe4key_desc[2], output_desc, st;
    if (pipe(pipe4key_desc) == -1 || pipe(pipe4orig_desc) == -1) {
        printf("Pipe opening failure\n");
        exit(EXIT_FAILURE);
    }
    switch (id)
    {
    case -1:
        printf("Fork in main process broke\n");
        exit(EXIT_FAILURE);
    case 0:
    {
        int id2 = fork();
        char* ar[3];
        ar[0] = command;
        ar[2] = NULL;
        close(pipe4key_desc[0]);
        close(pipe4orig_desc[0]);
        switch (id2)
        {
        case -1:
            printf("Fork in subprocess broke\n");
            exit(EXIT_FAILURE);
        case 0:
            close(pipe4key_desc[1]);
            if (dup2(pipe4orig_desc[1], STDOUT_FILENO) < 0) {
                close(pipe4orig_desc[1]);
                fprintf(stderr, "Fucking pipe is fucked up, who else could do this if fucking pipe wasn't, ha?\n");
                exit(EXIT_FAILURE);
            }
            close(pipe4orig_desc[1]);
            ar[1] = argv[1];
            execv(command, ar);
            exit(EXIT_FAILURE); //original
        default:
            close(pipe4orig_desc[1]);
            if (dup2(pipe4key_desc[1], STDOUT_FILENO) < 0) {
                close(pipe4key_desc[1]);
                fprintf(stderr, "Fucking pipe is fucked up, who else could do this if fucking pipe wasn't, ha?\n");
                exit(EXIT_FAILURE);
            }
            close(pipe4key_desc[1]);
            ar[1] = argv[2];
            if (WEXITSTATUS(st) == EXIT_FAILURE) {
                fprintf(stderr, "Fucking fork in your ass\n");
                return EXIT_FAILURE;
            }
            execv(command, ar); // returns control unlike execvp
            exit(EXIT_FAILURE); //key
        }
        break;
    }
    }
    close(pipe4orig_desc[1]);
    close(pipe4key_desc[1]);
    if ((0 > wait(&st) || WEXITSTATUS(st) == EXIT_FAILURE) 
    && (0 > wait(&st) || WEXITSTATUS(st) == EXIT_FAILURE)) {
        close(pipe4key_desc[0]);
        close(pipe4orig_desc[0]);
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
        off += PIPE_BUF;
        biba_buf = (char *)realloc((void *)biba_buf, off);
        check_mem((void *)biba_buf);
    } while ((biba = read(pipe4key_desc[0], biba_buf + off - PIPE_BUF, PIPE_BUF)) == PIPE_BUF);
    off += biba - PIPE_BUF;
    close(pipe4key_desc[0]);
    if (biba < 0 || !off) {
        if (!off) printf("Reading key failure!!!\n");
        else perror("Reading key error");
        close(pipe4orig_desc[0]);
        exit(EXIT_FAILURE);
    }
    printf("Reading key completed: %lu symbols have been read!\n", off);
    boba_buf = (char *)malloc(off);
    check_mem((void *)boba_buf);
    biba = 0;
    while ((boba = read(pipe4orig_desc[0], biba_buf, off)) > 0) {
        for (int i = 0; i < boba; ++i) boba_buf[i] ^= biba_buf[i];
        if (write(output_desc, boba_buf, boba) < 0) {
            close(output_desc);
            close(pipe4orig_desc[0]);
            perror("Writing error");
            exit(EXIT_FAILURE);
        }
        biba += boba;
    }
    if (boba < 0) {
        printf("Reading original failure!!!");
        close(pipe4orig_desc[0]);
        exit(EXIT_FAILURE);
    }
    printf("Reading original completed: %lu symbols have been read!\n", biba);
    printf("Success!!!\n");
    free(boba_buf);
    free(biba_buf);
    close(pipe4orig_desc[0]);
    exit(EXIT_SUCCESS);
}