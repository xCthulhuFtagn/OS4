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
    if (argc == 1 && strcmp(argv[1], "--help") == 0) {
        printf("Right input: ./scrambler {original_file} {key_file} {output_file}\n");
        exit(EXIT_SUCCESS);
    }
    if (argc != 4) {
        printf("Wrong number of arguments!\n");
        exit(EXIT_FAILURE);
    }
    char command[BUFSIZ];
    getcwd(command, BUFSIZ);
    strcat(command, "/printer");
    int id, pipe4orig_desc[2], pipe4key_desc[2], output_desc, st;
    if (pipe(pipe4key_desc) == -1 || pipe(pipe4orig_desc) == -1) {
        printf("Pipe opening failure\n");
        exit(EXIT_FAILURE);
    }
    id = fork();
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
        case 0: //original
            close(pipe4key_desc[0]);
            close(pipe4key_desc[1]);
            if (dup2(pipe4orig_desc[1], STDOUT_FILENO) < 0) {
                close(pipe4orig_desc[1]);
                fprintf(stderr, "Duplication of didn't work for original\n");
                exit(EXIT_FAILURE);
            }
            ar[1] = argv[1];
            execv(command, ar);
            close(pipe4orig_desc[1]);
            exit(EXIT_FAILURE);
        default: //key
            close(pipe4orig_desc[0]);
            close(pipe4orig_desc[1]);
            close(pipe4key_desc[0]);
            if (dup2(pipe4key_desc[1], STDOUT_FILENO) < 0) {
                close(pipe4key_desc[1]);
                fprintf(stderr, "Duplication didn't work for key\n");
                exit(EXIT_FAILURE);
            }
            close(pipe4key_desc[1]);
            ar[1] = argv[2];
            execv(command, ar);
            exit(EXIT_FAILURE); //key
        }
        break;
    }
    }
    close(pipe4orig_desc[1]);
    close(pipe4key_desc[1]);
    if ((0 > wait(&st) || WEXITSTATUS(st) == EXIT_FAILURE) 
    || (0 > wait(&st) || WEXITSTATUS(st) == EXIT_FAILURE)) {
        if (errno != ECHILD) {
            close(pipe4key_desc[0]);
            close(pipe4orig_desc[0]);
            perror("Something went wrong");
            exit(EXIT_FAILURE);
        }
    }
    size_t read_key, read_orig;
    char * key_buf, * orig_buf;
    key_buf = NULL;
    if ((output_desc = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR)) < 0) {
        printf("Output file could not be opened\n");
        exit(EXIT_FAILURE);
    }
    size_t off = 0;
    do {
        off += PIPE_BUF;
        key_buf = (char *)realloc((void *)key_buf, off);
        check_mem((void *)key_buf);
    } while ((read_key = read(pipe4key_desc[0], key_buf + off - PIPE_BUF, PIPE_BUF)) == PIPE_BUF);
    off += read_key - PIPE_BUF;
    close(pipe4key_desc[0]);
    if (read_key < 0 || !off) {
        if (!off) printf("Reading key failure!!!\n");
        else perror("Reading key error");
        close(pipe4orig_desc[0]);
        exit(EXIT_FAILURE);
    }
    printf("Reading key completed: %lu symbols have been read!\n", off);
    orig_buf = (char *)malloc(off);
    check_mem((void *)orig_buf);
    read_key = 0;
    while ((read_orig = read(pipe4orig_desc[0], orig_buf, off)) > 0) {
        for (int i = 0; i < read_orig; ++i) orig_buf[i] ^= key_buf[i];
        if (write(output_desc, orig_buf, read_orig) < 0) {
            close(output_desc);
            close(pipe4orig_desc[0]);
            perror("Writing error");
            exit(EXIT_FAILURE);
        }
        read_key += read_orig;
    }
    if (read_orig < 0) {
        printf("Reading original failure!!!");
        close(pipe4orig_desc[0]);
        exit(EXIT_FAILURE);
    }
    printf("Reading original completed: %lu symbols have been read!\n", read_key);
    printf("Success!!!\n");
    free(orig_buf);
    free(key_buf);
    close(pipe4orig_desc[0]);
    exit(EXIT_SUCCESS);
}